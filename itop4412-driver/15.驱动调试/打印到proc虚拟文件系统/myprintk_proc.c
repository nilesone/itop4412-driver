#include <linux/module.h>
#include <linux/version.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define PRINTK_BUFF_SIZE 1024

static char printk_buff[PRINTK_BUFF_SIZE];
static char printk_tmp[PRINTK_BUFF_SIZE];
static unsigned int mymsg_r = 0;			//读取数据时这个变量不变，只有buff满了舍弃数据时才会变
static unsigned int mymsg_read_tmp = 0;		//读取数据时这个变量变化，跟上面的变量分开是为了每次都可以读取全部数据
static unsigned int mymsg_w = 0;
static DECLARE_WAIT_QUEUE_HEAD(mymsg_waitq);

static int ismylog_full(void)
{
	return ((mymsg_w + 1) % PRINTK_BUFF_SIZE == mymsg_r);
}

static int ismylog_empty(void)
{
	return (mymsg_read_tmp == mymsg_w);
}

static void mylog_putc(char c)
{
	if (ismylog_full())
	{
		/* buff已满，丢弃一个数据 */
		mymsg_r = (mymsg_r + 1) % PRINTK_BUFF_SIZE;
		if ((mymsg_read_tmp + 1) % PRINTK_BUFF_SIZE == mymsg_r)		//mymsg_r已变，mymsg_read_tmp也要变，否则打印的消息会不全
		{
			mymsg_read_tmp = mymsg_r;
		}
	}
	
	printk_buff[mymsg_w] = c;
	mymsg_w = (mymsg_w + 1) % PRINTK_BUFF_SIZE;
}

static int mylog_getc(char *p)
{
	if(ismylog_empty())
	{
		return 0;
	}
	
	*p = printk_buff[mymsg_read_tmp];
	mymsg_read_tmp = (mymsg_read_tmp + 1) % PRINTK_BUFF_SIZE;
	return 1;
}

int myprintk(const char *fmt, ...)
{
	va_list args;
	int cnt, i;
	
	va_start(args, fmt);
	cnt = vsnprintf(printk_tmp, INT_MAX, fmt, args);
	va_end(args);

	for(i = 0; i < cnt; i++)
	{
		mylog_putc(printk_tmp[i]);
	}
	/* 有程序调用了myprintk函数，把数据存进buff后再唤醒 */
	wake_up_interruptible(&mymsg_waitq);

	return cnt;
}
EXPORT_SYMBOL(myprintk);

static ssize_t mymsg_read(struct file *file, char __user *buf,
			 size_t count, loff_t *ppos)
{
	char c;
	int error = 0;
	int i = 0;
	if ((file->f_flags & O_NONBLOCK) && ismylog_empty())
		return -EAGAIN;
	
	/* 系统在cat /proc/mymsg时，会多次调用mymsg_read函数，最终还是会阻塞到这里 */
	wait_event_interruptible(mymsg_waitq, !ismylog_empty());
	
	while(i<count)
	{
		if(mylog_getc(&c))
		{
			error = __put_user(c,buf);
			buf++;
			i++;
		}
		else 
			break;	
	}
	
	if (!error)
		error = i;
	
	return error;
}

static int mymsg_open(struct inode * inode, struct file * file)
{
	mymsg_read_tmp = mymsg_r;
	return 0;
}

static const struct file_operations proc_mymsg = {
	.read		= mymsg_read,
	.open		= mymsg_open,
};

static int myprintk_proc_init(void)
{
	/* 在/proc文件目录下创建一个mymsg文件    	 	*/
	proc_create("mymsg", S_IRUSR, NULL, &proc_mymsg);
	return 0;
}

static void myprintk_proc_exit(void)
{
	remove_proc_entry("mymsg", NULL);
}


module_init(myprintk_proc_init);
module_exit(myprintk_proc_exit);

MODULE_LICENSE("GPL");

