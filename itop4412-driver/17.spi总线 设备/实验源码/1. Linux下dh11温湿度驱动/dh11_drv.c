
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

#include <linux/delay.h>
#include <linux/irqflags.h>

#include <linux/wait.h>
#include <linux/sched.h>

#define data_pin 	EXYNOS4_GPK1(0)

struct timer_list dh11_timer;
static struct work_struct dh11_work;
static unsigned char dh11_data[5];

extern int lcd_spi_write(unsigned int page, unsigned int col, unsigned char *data, unsigned int count);


static void dh11_gpio_init(void)
{
	int ret;
	ret = gpio_request(data_pin, "dh11_data");       
	if(ret < 0)
	{
		printk("gpio_request data_pin fail!\n");
		return;
	}
	
}

static void dh11_work_func(struct work_struct *work)
{
	int i, j;
	int count = 0;
	unsigned char crc=0;
	unsigned long flags;
	unsigned char buff[32];

	/* dh11对时序要求高，所以要锁中断，          在读取数据时不可以做打印信息之类占用系统资源的动作，否则会出错 */
	local_irq_save(flags);
	
	/* 这部分看dh11中文说明书p3 */
	/* 1.主机发开始信号，主机至少拉低18ms */
	s3c_gpio_cfgpin(data_pin, S3C_GPIO_OUTPUT);
	gpio_set_value(data_pin,0);
	mdelay(20);
	/* 2.主机拉高等待20-40us */
	gpio_set_value(data_pin,1);
	udelay(30);
	/* 3.4412设为从机模式(输入模式) */
	s3c_gpio_cfgpin(data_pin, S3C_GPIO_INPUT);
	/* 4.检测到DH11的响应信号 */
	if(gpio_get_value(data_pin) == 0)
	{
		/* 4.1 dh11响应信号 */
		while(gpio_get_value(data_pin) == 0)
		{
			udelay(5);
			count++;
			if(count > 20)
				return;
		}
		
		/* 4.2 dh11把信号拉高 */
		count = 0;
		while(gpio_get_value(data_pin) == 1)
		{
			udelay(5);
			count++;
			if(count > 20)
				return;
		}
		
		/* 5.开始传输数据 一共40个bit      先传高位  
		 *
		 *	数据格式:8bit湿度整数数据+8bit湿度小数数据
		 *			+8bi温度整数数据+8bit温度小数数据
         *			+8bit校验和
         *
		 */ 
		for(i=0;i<5;i++)
		{
			for(j=0;j<8;j++)
			{
				dh11_data[i] <<= 1;
			/* 5.1 数据开始信号 */
				count = 0;
				while(gpio_get_value(data_pin) == 0)
				{
					udelay(5);
					count++;
					if(count > 20)
						return;
				}
				udelay(40);
			/* 5.2 35us后还是低电平，这一位为0，否则为1 */
				if(gpio_get_value(data_pin) == 0)
				{
					dh11_data[i] &= ~(1<<0);
				}
				else
				{
					dh11_data[i] |= (1<<0);				
					while(gpio_get_value(data_pin) == 1)
					{
						udelay(5);
						count++;
						if(count > 20)
							return;
					}
				}
				
			}
		}
		
		local_irq_restore(flags);
		
		/* 6.检验数据是否正确 */
		/* 
		 *  数据传送正确时校验和数据等于
		 *  8bit湿度整数数据+8bit湿度小数数据
		 *	+8bi温度整数数据+8bit温度小数数据”所得结果的末8位 
		 */
		 
		for(i=0;i<4;i++)
		{
			crc += dh11_data[i];
		}
		if(crc == dh11_data[4])
		{
			sprintf(buff, "T = %d, RH = %d", dh11_data[2], dh11_data[0]);
			lcd_spi_write(0, 0, buff, 15);
		}
		else
		{
			printk("read temperature and humidity fail(CRC)!\n");
		}
		
	}
	else
	{
		printk("read temperature and humidity fail!\n");
		return;
	}
	
}


static void dh11_timer_hander(unsigned long data)
{
	/* 启动一个工作对列来读取一次温湿度，由于采集数据时间比较久，所以用工作队列的方式 */
	schedule_work(&dh11_work);
	/* 下次读取时间 */
	mod_timer(&dh11_timer, jiffies + HZ);
}


static int dh11_drv_init(void)
{
	/* 使用内核定时器来每隔一段时间读取一次温度和湿度 */
	init_timer(&dh11_timer);
	dh11_timer.function = dh11_timer_hander; 	/* timer handler */
	/* 每五秒读一次数值 HZ=200       */
	dh11_timer.expires = jiffies + 5 * HZ;
	
	/* dh11初始化 */
	dh11_gpio_init();
	
	/* 初始化工作队列 */
	INIT_WORK(&dh11_work, dh11_work_func);
	
	add_timer(&dh11_timer);

	return 0;
}

static void dh11_drv_exit(void)
{
	del_timer(&dh11_timer);
	cancel_work_sync(&dh11_work);
	gpio_free(data_pin);
}

module_init(dh11_drv_init);
module_exit(dh11_drv_exit);

MODULE_LICENSE("GPL");

