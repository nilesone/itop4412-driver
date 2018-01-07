#include <linux/init.h>
#include <linux/module.h>

#include <linux/kernel.h>
#include <linux/fs.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <mach/regs-gpio.h>
#include <asm/io.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
//copy_to_user的头文件
#include <asm/uaccess.h>

#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/sched.h>

//ioctl的cmd不按标准的cmd命令格式的话，要从3开始，因为2在内核函数调用过程中(代表特殊含义)会挂掉的
#define ker_r8   3
#define ker_r16  4
#define ker_r32  5

#define ker_w8   6
#define ker_w16  7
#define ker_w32  8


static unsigned int major;
static struct class *ker_rw_cls;

static long ker_rw_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{

	unsigned int buff[2];	
	unsigned int val;
	unsigned int phy_addr;
	volatile unsigned char  *p8;
	volatile unsigned short *p16;	
	volatile unsigned int   *p32;

	copy_from_user(buff, (void __user *)arg, 8);
	phy_addr = buff[0];
	val      = buff[1];

	p8  = (unsigned char  *)ioremap(phy_addr, 4);
	p16 = (unsigned short *)p8;
	p32 = (unsigned int   *)p8;
	
	switch(cmd)
	{
		case ker_r8:
		{
			val = *p8;
			copy_to_user((void __user *)(arg + 4), &val, 4);
			break;
		}
		case ker_r16:
		{
			val = *p16;
			copy_to_user((void __user *)(arg + 4), &val, 4);
			break;
		}
		case ker_r32:
		{
			val = *p32;
			copy_to_user((void __user *)(arg + 4), &val, 4);
			break;
		}

		case ker_w8:
		{
			*p8 = val;
			break;
		}
		case ker_w16:
		{
			*p16 = val;
			break;
		}
		case ker_w32:
		{
			*p32 = val;
			break;
		}
		default:
		{
			printk("error!\n");
			break;
		}
	}

	iounmap(p8);
	
	return 0;
}

static int ker_rw_open(struct inode *inode, struct file *file)
{
	return 0;
}


static struct file_operations ker_rw_ops = {
	.owner 			= THIS_MODULE,
	.open 			= ker_rw_open,
	.unlocked_ioctl = ker_rw_ioctl,
};

static int ker_rw_init(void)
{
	major = register_chrdev(0, "ker_rw", &ker_rw_ops);
	ker_rw_cls = class_create(THIS_MODULE, "ker_rw");
	device_create(ker_rw_cls, NULL, MKDEV(major, 0), NULL, "ker_rw");
	return 0;
}

static void ker_rw_exit(void)
{
	device_destroy(ker_rw_cls, MKDEV(major, 0));
	class_destroy(ker_rw_cls);
	unregister_chrdev(major, "ker_rw");
}

module_init(ker_rw_init);
module_exit(ker_rw_exit);

MODULE_LICENSE("GPL");

