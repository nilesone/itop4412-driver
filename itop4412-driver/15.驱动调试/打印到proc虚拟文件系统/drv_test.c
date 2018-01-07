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
extern int myprintk(const char *fmt, ...);

static int test_init(void)
{
	int cnt = 0;
	for(; cnt < 1024; cnt++)
	{
		myprintk("Hello,this is test_init %d\n", cnt);
	}
	return 0;
}

static void test_exit(void)
{
	int cnt = 0;
	myprintk("Hello,this is test_exit %d\n", ++cnt);
}


module_init(test_init);
module_exit(test_exit);

MODULE_LICENSE("GPL");

