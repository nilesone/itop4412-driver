#include <linux/init.h>
#include <linux/module.h>

#include <linux/kernel.h>
#include <linux/fs.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
//#include <mach/gpio-bank.h>

#include <mach/gpio-exynos4.h>

#include <mach/regs-gpio.h>
#include <asm/io.h>
#include <linux/regulator/consumer.h>

#include <linux/delay.h>
#include <linux/irqflags.h>

#include <linux/wait.h>
#include <linux/sched.h>


#define SCL		EXYNOS4_GPC1(1)
#define SDAOUT	EXYNOS4_GPC1(4) 
#define SDAIN	EXYNOS4_GPC1(3) 


static int spi_gpio_init(void)
{
	int ret;

	ret = gpio_request(SCL, "SPI_SCL");       
	if(ret < 0)
	{
		printk("gpio_request SCL fail!\n");
		return -1;
	}
	s3c_gpio_cfgpin(SCL, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(SCL, S3C_GPIO_PULL_UP);
	s5p_gpio_set_drvstr(SCL, S5P_GPIO_DRVSTR_LV4);
	gpio_set_value(SCL, 1);
	
	ret = gpio_request(SDAOUT, "SPI_SDAOUT");       
	if(ret < 0)
	{
		printk("gpio_request SDAOUT fail!\n");
		return -1;
	}
	s3c_gpio_cfgpin(SDAOUT, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(SDAOUT, S3C_GPIO_PULL_UP);
	s5p_gpio_set_drvstr(SDAOUT, S5P_GPIO_DRVSTR_LV4);
	gpio_set_value(SDAOUT, 1);

	ret = gpio_request(SDAIN, "SPI_SDAIN");       
	if(ret < 0)
	{
		printk("gpio_request SDAIN fail!\n");
		return -1;
	}
	s3c_gpio_cfgpin(SDAIN, S3C_GPIO_INPUT);
	s3c_gpio_setpull(SDAIN, S3C_GPIO_PULL_NONE);
	
	return 0;
}



void write_spi_byte(unsigned char data)
{
	int i;

	for(i=0;i<8;i++)
	{
		gpio_set_value(SCL, 0);
		gpio_set_value(SDAOUT, (data & 0x80) ? 1 : 0);
		gpio_set_value(SCL, 1);
		data <<= 1;
	}
	
}
EXPORT_SYMBOL(write_spi_byte);

unsigned char read_spi_byte(void)
{
	int i;
	unsigned char val = 0;
	for(i=0;i<8;i++)
	{
		val <<= 1;
		gpio_set_value(SCL, 0);
		if(gpio_get_value(SDAIN) == 1)
		{
			val |= 1;
		}
		gpio_set_value(SCL, 1);
	}
	return val;
}
EXPORT_SYMBOL(read_spi_byte);

static int gpio_spi_init(void)
{
	spi_gpio_init();

	return 0;
}

static void gpio_spi_exit(void)
{
	
	gpio_free(SCL);
	gpio_free(SDAIN);
	gpio_free(SDAOUT);

}

module_init(gpio_spi_init);
module_exit(gpio_spi_exit);

MODULE_LICENSE("GPL");


