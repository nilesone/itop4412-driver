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

#include "lcdfont.h"

#define RST		EXYNOS4_GPK1(0)
#define A0		EXYNOS4_GPA1(5)
#define CS		EXYNOS4_GPB(4)

extern 	void write_spi_byte(unsigned char data);
extern 	unsigned char read_spi_byte(void);


static int st7567_gpio_init(void)
{
	int ret;
		
	ret = gpio_request(RST, "st7567_rst");		 
	if(ret < 0)
	{
		printk("gpio_request RST fail!\n");
		return -1;
	}
	s3c_gpio_cfgpin(RST, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(RST, S3C_GPIO_PULL_UP);
	s5p_gpio_set_drvstr(RST, S5P_GPIO_DRVSTR_LV4);
	gpio_set_value(RST, 1);

	ret = gpio_request(A0, "st7567_A0");	   
	if(ret < 0)
	{
		printk("gpio_request A0 fail!\n");
		return -1;
	}
	s3c_gpio_cfgpin(A0, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(A0, S3C_GPIO_PULL_UP);
	s5p_gpio_set_drvstr(A0, S5P_GPIO_DRVSTR_LV4);	
	gpio_set_value(A0, 1);

	ret = gpio_request(CS, "st7567_CS");	   
	if(ret < 0)
	{
		printk("gpio_request CS fail!\n");
		return -1;
	}
	s3c_gpio_cfgpin(CS, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(CS, S3C_GPIO_PULL_UP);
	s5p_gpio_set_drvstr(CS, S5P_GPIO_DRVSTR_LV4);
	gpio_set_value(CS, 1);

	return 0;

}

static void st7567_reset(void)
{
	gpio_set_value(RST, 0);
	mdelay(2);
	gpio_set_value(RST, 1);
	mdelay(4);
	gpio_free(RST);
}

static void write_spi_lcd_cmd(unsigned char cmd)
{
	gpio_set_value(A0, 0);
	gpio_set_value(CS, 0);

	write_spi_byte(cmd);


	gpio_set_value(CS, 1);

}

static void write_spi_lcd_data(unsigned char data)
{
	gpio_set_value(A0, 1);
	gpio_set_value(CS, 0);

	write_spi_byte(data);

	gpio_set_value(CS, 1);

}



static void clear_screen(unsigned char data)
{
	int i,j;
	for(i = 0; i < 8; i++)
	{
	    write_spi_lcd_cmd(0xb0+i);
	    for(j = 0; j < 128; j++)
	    {
	        write_spi_lcd_cmd(0x10 + ((j & 0xf0) >> 4));
	        write_spi_lcd_cmd(0x00 + (j & 0x0f));
	        write_spi_lcd_data(data);
	    }
	}
}

static void gpio_spi_write_char_bit(unsigned char bit)
{
	write_spi_lcd_data(bit);
}

static void gpio_spi_write_char(unsigned int page, unsigned int col, unsigned char data_char)
{
	int i;
	unsigned int buff = data_char - ' ';

	write_spi_lcd_cmd(0xb0 + page);
	for(i=0; i<8; i++)
	{
		write_spi_lcd_cmd(0x10 + (((col + i) & 0xf0) >> 4));
		write_spi_lcd_cmd(0x00 + ((col + i) & 0x0f));
		gpio_spi_write_char_bit(oled_asc2_8x16[buff][0 + i]);
	}

	write_spi_lcd_cmd(0xb0 + page + 1);
	for(i=0; i<8; i++)
	{
		write_spi_lcd_cmd(0x10 + (((col + i) & 0xf0) >> 4));
		write_spi_lcd_cmd(0x00 + ((col + i) & 0x0f));
		gpio_spi_write_char_bit(oled_asc2_8x16[buff][8 + i]);
	}
}


int lcd_spi_write(unsigned int page, unsigned int col, unsigned char *data, unsigned int count)
{
	int i;

	for(i=0; i<count; i++)
	{
		if((col + 8) > 128)
		{
			page += 2;
			col = 0;
		}
		
		if((page + 1) > 7)
			return -1;
		
		gpio_spi_write_char(page, col, data[i]);
		col  += 8; 
	}

	return 0;
}

EXPORT_SYMBOL(lcd_spi_write);


static void st7567_init(void)
{
	int ret;

    ret = st7567_gpio_init();

	if(ret < 0)
	{
		return;
	}
    
    mdelay(2);
    st7567_reset(); 		 //复位 LCD
    mdelay(100);
    /* lcd初始化，看官方提供的例程 */
    write_spi_lcd_cmd(0xe2); //Soft rest
    write_spi_lcd_cmd(0xa2); //SET LCD bias(A2-1/9bias; A3-1/7bias)
    write_spi_lcd_cmd(0xa0); //SET ADC NORMAL(OB-POR seg0-00h) A0: NORMAL
                             //A1: REVERSE
    write_spi_lcd_cmd(0xc8); //SET COM OUTPUT SCAN
                             //DIRECTION(0XXXB-NORMAL)-POR COM63-->COM0
    write_spi_lcd_cmd(0xa4); // SET DISPLAY NORMAL (0B-NORMAL)-POR
                             //A4:NORMAL A5:ENTIRE DISPLAY ON
    write_spi_lcd_cmd(0xa6); //SET NORMAL DISPLAY MODE(0B-NORMAL)
                             //A6:NORMAL A7:REVERSE
    write_spi_lcd_cmd(0x25); //SET INTERNAL REGULATOR RESDASTOR
                             //RATIO(100B)-POR
    write_spi_lcd_cmd(0x81);
    write_spi_lcd_cmd(0x1a); // SET CONTRAST CONTROL
                             //REGISTER(00,0000H-11,1111H) 30 对比度请修改此值，调浓增大此值，反之调淡。
    write_spi_lcd_cmd(0x2f); //SET POWER CONTROL REGISTER (ALL
                             //INTERNAL)
    write_spi_lcd_cmd(0x40); //end of initialzation
    write_spi_lcd_cmd(0xaf); //DisPlay On

}


static int lcd_spi_init(void)
{
	unsigned char val;
	st7567_init();
	clear_screen(0);
	lcd_spi_write(0, 0, "ni hao!", 7);
	val = read_spi_byte();
	printk("val = %d\n", val);
	
	return 0;
}

static void lcd_spi_exit(void)
{
	gpio_free(RST);
	gpio_free(A0);
	gpio_free(CS);

}

module_init(lcd_spi_init);
module_exit(lcd_spi_exit);

MODULE_LICENSE("GPL");



