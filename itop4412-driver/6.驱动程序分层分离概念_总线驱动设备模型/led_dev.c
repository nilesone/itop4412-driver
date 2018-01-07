
#include <linux/module.h>
#include <linux/version.h>

#include <linux/init.h>

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>





static void led_dev_release(struct device *dev)
{
	
}

struct resource led_res[] = {
	[0] = {
		.start = 0x11000060,
		.end   = 0x11000060 + 16 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = 1,
		.end   = 1,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device led_dev = {
	.name = "myled",
	.id = -1,
	.num_resources = ARRAY_SIZE(led_res),
	.resource = led_res,
	.dev = {
		.release = led_dev_release, /* Passed to driver */
	},
};



static int led_dev_init(void)
{
	platform_device_register(&led_dev);
	return 0;
}
static void led_dev_exit(void)
{
	platform_device_unregister(&led_dev);
}

module_init(led_dev_init);
module_exit(led_dev_exit);
MODULE_LICENSE("GPL");

