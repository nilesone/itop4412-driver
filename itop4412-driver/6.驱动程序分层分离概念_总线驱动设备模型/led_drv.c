/* 分配/设置/注册一个platform_driver */

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

static unsigned int major;
static struct class *cls;
static volatile unsigned int *gpiocon;
static volatile unsigned int *gpiodat;
static int pin;

static int led_open(struct inode *inode, struct file *file)
{
	*gpiocon &= ~(0xf<<(pin*4));
	*gpiocon |= (0x1<<(pin*4));
	*gpiodat &= ~(0x1<<(pin*1));
	return 0;
}

static ssize_t led_write(struct file *file, const char __user *buf, size_t count, loff_t * ppos)
{
	unsigned int to;
	copy_from_user(&to, buf, count);
	if(to)
	{
		*gpiodat |= (0x1<<(pin*1)); 
	}
	else
	{
		*gpiodat &= ~(0x1<<(pin*1)); 
	}
	return 0;
}

static int led_release(struct inode * inode, struct file * filp)
{
	printk("led_release!\n");
	return 0;
}

struct file_operations led_ops = {
	.owner		= THIS_MODULE,
	.open		= led_open,
	.write		= led_write,
	.release	= led_release,
};

static int led_probe(struct platform_device *pdev)
{
	struct resource *res;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	gpiocon = ioremap(res->start, res->end - res->start + 1);
	gpiodat = gpiocon + 1;
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	pin = res->start;
	
	major = register_chrdev(0, "led", &led_ops);
	
	cls = class_create(THIS_MODULE, "myclass");
	device_create(cls, NULL, MKDEV(major, 0), NULL, "myled");
	
	printk("led_probe found led!\n");
	return 0;
}

static int led_remove(struct platform_device *pdev)
{
	unregister_chrdev(major, "led");
	device_destroy(cls, MKDEV(major, 0));
	class_destroy(cls);
	printk("led_remove found led!\n");
	return 0;
}



struct platform_driver led_drv = {
	.probe		= led_probe,
	.remove		= led_remove,
	.driver= {
		.name 	= "myled",
	},
};
static int led_drv_init(void)
{
	platform_driver_register(&led_drv);
	return 0;
}
static void led_drv_exit(void)
{
	platform_driver_unregister(&led_drv);
}

module_init(led_drv_init);
module_exit(led_drv_exit);
MODULE_LICENSE("GPL");


