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
//输入子系统头文件
#include <linux/input.h>

#define DPRINTK(x...) printk("POLLKEY_CTL DEBUG:" x)
#define DRIVER_NAME "pollkey"


static struct input_dev *button_input; 

struct key_desc{
	int irq;
	char *name;
	unsigned int key_pin;
	unsigned char key_value;
};

struct key_desc *irq_pin;

struct timer_list button_timer;

static struct key_desc button_desc[4] = {
	{IRQ_EINT(9),  "s1", EXYNOS4_GPX1(1), KEY_L},
	{IRQ_EINT(10), "s2", EXYNOS4_GPX1(2), KEY_S},
	{IRQ_EINT(17), "s3", EXYNOS4_GPX2(1), KEY_LEFTSHIFT},
	{IRQ_EINT(27), "s4", EXYNOS4_GPX3(3), KEY_ENTER},
	
};


static irqreturn_t buttons_irq(int irq, void *dev_id)
{
	/* 10ms后启动定时器 */
	irq_pin = (struct key_desc *)dev_id;
	mod_timer(&button_timer, jiffies+HZ/100);
	return IRQ_RETVAL(IRQ_HANDLED);
}

static void button_timeout_handler(unsigned long data)
{
	unsigned char value;
	if(!irq_pin)
	{
		return;
	}
	value = gpio_get_value(irq_pin->key_pin);
	if(value)	//松开
	{
		input_event(button_input, EV_KEY, irq_pin->key_value, 0);
		input_sync(button_input);
	}else      //按下
	{
		input_event(button_input, EV_KEY, irq_pin->key_value, 1);
		input_sync(button_input);
	}
		
}

static int buttons_init(void)

{
    int i, ret;
	printk("buttons_init\n");
	button_input = input_allocate_device();
	
	set_bit(EV_KEY, button_input->evbit);

	set_bit(KEY_L, button_input->keybit);
	set_bit(KEY_S, button_input->keybit);
	set_bit(KEY_ENTER, button_input->keybit);
	set_bit(KEY_LEFTSHIFT, button_input->keybit);
	
	ret = input_register_device(button_input);
	if(ret)
		{
		printk("input_register_device fail!\n");
	}

	for(i=0;i<4;i++)
	{
			ret = request_irq(button_desc[i].irq, buttons_irq, IRQ_TYPE_EDGE_BOTH, button_desc[i].name, &button_desc[i]);
			if(ret)
				{
					printk("Request_irq %s fail!\n", button_desc[i].name);
				}
	}
	init_timer(&button_timer);
	button_timer.function = button_timeout_handler;
	add_timer(&button_timer);
	
	return 0;
}
static void buttons_exit(void)

{
	int i;
		for(i=0;i<4;i++)
	{
			free_irq(button_desc[i].irq, &button_desc[i]);
	}
	del_timer(&button_timer);
	input_unregister_device(button_input);
	input_free_device(button_input);
}


module_init(buttons_init);
module_exit(buttons_exit);
MODULE_LICENSE("GPL");


