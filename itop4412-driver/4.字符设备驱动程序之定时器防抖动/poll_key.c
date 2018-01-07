#include <linux/init.h>
#include <linux/module.h>

#include <linux/kernel.h>
#include <linux/fs.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
//#include <mach/gpio-bank.h>
#include <mach/regs-gpio.h>
#include <asm/io.h>
#include <linux/regulator/consumer.h>
//#include "gps.h"
#include <linux/delay.h>

//copy_to_user的头文件
#include <asm/uaccess.h>

#include <linux/irq.h>
#include <linux/interrupt.h>
//poll机制头文件
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/sched.h>

#define DPRINTK(x...) printk("POLLKEY_CTL DEBUG:" x)
#define DRIVER_NAME "pollkey"

struct key_desc{
	unsigned int key_num;
	unsigned char key_value;
};

static struct key_desc button_desc[2] = {
	{EXYNOS4_GPX1(1), 0xA0},
	{EXYNOS4_GPX1(2), 0xB0},
};
struct key_desc *irq_pin;
static volatile int flag = 0;
static DECLARE_WAIT_QUEUE_HEAD(button_waitq);
static struct fasync_struct *button_async;
//static atomic_t button_atomic= ATOMIC_INIT(0);
struct semaphore button_sem;
struct timer_list button_timer;
unsigned char key_value;

static irqreturn_t handler_irq(int irq, void *dev_id)
{
	irq_pin = (struct key_desc *)dev_id;
	mod_timer(&button_timer, jiffies + HZ/100);
	//kill_fasync(&button_async, SIGIO, POLL_IN);
	return IRQ_HANDLED;
}

int pollkey_open(struct inode *inode,struct file *filp)
{
/*	if(!atomic_dec_and_test(&button_atomic))
	{
		atomic_inc(&button_atomic);
		return -EBUSY;
	}
*/
	if(filp->f_flags & O_NONBLOCK)
	{
		if(down_trylock(&button_sem))
		return -EBUSY;
	}
	else{
		down(&button_sem);
	}
	
	DPRINTK("Device Opened Success!\n");
	return nonseekable_open(inode,filp);
}

int pollkey_release(struct inode *inode,struct file *filp)
{
	//atomic_inc(&button_atomic);
	up(&button_sem);
	DPRINTK("Device Closed Success!\n");
	return 0;
}

static ssize_t pollkey_read(struct file *filp, char __user *buff, size_t size, loff_t *ppos)
{
	
	
	if(filp->f_flags & O_NONBLOCK)
	{
		if(!flag)
			return -EAGAIN;
	}
	else{
		wait_event_interruptible(button_waitq, flag);
	}
	copy_to_user(buff, &key_value, 1);
	flag = 0;
	return 0;
}

static unsigned pollkey_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;
	DPRINTK("poll_wait come on!\n");
	poll_wait(file, &button_waitq, wait);
	if(flag)
		mask |= POLLIN | POLLRDNORM;
	return mask;
}

static int pollkey_fasync(int fd, struct file *file, int on)
{
	return fasync_helper(fd, file, on, &button_async);
}

static struct file_operations pollkey_ops = {
	.owner 	= THIS_MODULE,
	.open 	= pollkey_open,
	.release= pollkey_release,
	.read 	= pollkey_read,
	.poll	= pollkey_poll,
	.fasync = pollkey_fasync,
};

static struct miscdevice pollkey_dev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.fops	= &pollkey_ops,
	.name	= "pollkey",
};


static int pollkey_probe(struct platform_device *pdev)
{
	int ret;
	sema_init(&button_sem, 1);
	char *banner = "pollkey Initialize\n";
	printk(banner);
	
	request_irq(IRQ_EINT(9), handler_irq, IRQ_TYPE_EDGE_BOTH, "irq_9", &button_desc[0]);
	request_irq(IRQ_EINT(10), handler_irq, IRQ_TYPE_EDGE_BOTH, "irq_10", &button_desc[1]);
	
	ret = misc_register(&pollkey_dev);
	return 0;
}

static int pollkey_remove (struct platform_device *pdev)
{
	free_irq(IRQ_EINT(9), &button_desc[0]);
	free_irq(IRQ_EINT(10), &button_desc[1]);
	misc_deregister(&pollkey_dev);	
	return 0;
}

static int pollkey_suspend (struct platform_device *pdev, pm_message_t state)
{
	DPRINTK("pollkey suspend:power off!\n");
	return 0;
}

static int pollkey_resume (struct platform_device *pdev)
{
	DPRINTK("pollkey resume:power on!\n");
	return 0;
}

static struct platform_driver pollkey_driver = {
	.probe = pollkey_probe,
	.remove = pollkey_remove,
	.suspend = pollkey_suspend,
	.resume = pollkey_resume,
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};

static void button_timeout_handler(unsigned long data)
{
	unsigned char ret;
	if(!irq_pin)
	{
		return;
	}
	flag = 1;
	wake_up_interruptible(&button_waitq);
	ret = gpio_get_value(irq_pin->key_num);
	key_value = irq_pin->key_value | ret;
		
}

static int __init pollkey_init(void)
{
	init_timer(&button_timer);
	//button_timer.expires = 0;
	//button_timer.data = (unsigned long)dev;
	button_timer.function = button_timeout_handler;		/* timer handler */
	add_timer(&button_timer);
	return platform_driver_register(&pollkey_driver);
}


static void __exit pollkey_exit(void)
{
	platform_driver_unregister(&pollkey_driver);
}


module_init(pollkey_init);
module_exit(pollkey_exit);
MODULE_LICENSE("Dual BSD/GPL");
