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
#include <linux/spi/spi.h>

#include <asm/uaccess.h>

#define OLED_CLEAR_ALL		0x100001
#define	OLED_CLEAR_PAGE		0x100002
#define OLED_SET_POS		0x100003

struct spi_oled_platform_data {
	unsigned int oled_dc;
	unsigned int rst;
};

static unsigned int major;
static struct class *oled_cls;
static struct spi_oled_platform_data *oled_platform_data;
static struct spi_device *oled_spi_device;
static unsigned char *oled_buff;
static unsigned int page_pos;
static unsigned int col_pos;

static void st7567_reset(void)
{
	int ret;
	ret = gpio_request(oled_platform_data->rst, "oled_rst");
	if (ret < 0)
	{
		printk("rst request fail!\n");
		return;
	}
	
	gpio_set_value(oled_platform_data->rst, 0);
	mdelay(2);
	gpio_set_value(oled_platform_data->rst, 1);
	mdelay(4);
	gpio_free(oled_platform_data->rst);

	ret = gpio_request(oled_platform_data->oled_dc, "st7567_A0");	   
	if(ret < 0)
	{
		printk("oled_dc request fail!\n");
		return;
	}
	s3c_gpio_cfgpin(oled_platform_data->oled_dc, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(oled_platform_data->oled_dc, S3C_GPIO_PULL_UP);
	s5p_gpio_set_drvstr(oled_platform_data->oled_dc, S5P_GPIO_DRVSTR_LV4);	
	gpio_set_value(oled_platform_data->oled_dc, 1);
}

static void write_spi_lcd_cmd(unsigned char cmd)
{
	gpio_set_value(oled_platform_data->oled_dc, 0);
	spi_write(oled_spi_device, &cmd, 1);
}

static void write_spi_lcd_data(unsigned char data)
{
	gpio_set_value(oled_platform_data->oled_dc, 1);
	spi_write(oled_spi_device, &data, 1);
}

/* 设置位置 */
static void oled_set_pos(unsigned int page, unsigned int col)
{
	page_pos = page;
	col_pos	 = col;
	write_spi_lcd_cmd(0xb0 + page);
	write_spi_lcd_cmd(0x10 + ((col & 0xf0) >> 4));
	write_spi_lcd_cmd(0x00 + (col & 0x0f));
}

static void clear_screen(void)
{
	int i,j;
	for(i = 0; i < 8; i++)
	{
	    write_spi_lcd_cmd(0xb0+i);
	    for(j = 0; j < 128; j++)
	    {
	        write_spi_lcd_cmd(0x10 + ((j & 0xf0) >> 4));
	        write_spi_lcd_cmd(0x00 + (j & 0x0f));
	        write_spi_lcd_data(0);
	    }
	}
}

/* 清一页屏幕 */
static void clear_pag(unsigned int page_cle)
{
	int i;
	write_spi_lcd_cmd(0xb0 + page_cle);
	for(i = 0; i < 128; i++)
	{
		write_spi_lcd_cmd(0x10 + ((i & 0xf0) >> 4));
		write_spi_lcd_cmd(0x00 + (i & 0x0f));
		write_spi_lcd_data(0);
	}

}

static void st7567_init(void)
{
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


ssize_t oled_write(struct file *file, const char __user *buff, size_t count, loff_t *offset)
{
	int ret, i;

	if (count > 4096)
		return -EIO;
	ret = copy_from_user(oled_buff, buff, count);
	if (ret != 0)
	{
		printk("copy_from_user fail!\n");
		return -EIO;
	}

	for(i=0; i<count; i++)								//把用户空间传过来的数据输出到oled屏幕上
	{
		write_spi_lcd_data(oled_buff[i]);
		col_pos++;
		if (col_pos > 127)
		{
			page_pos++;
			col_pos = 0;
			if(page_pos > 7)
			{
				printk("page_pos is cross-border!\n");
				return -EIO;
			}
			write_spi_lcd_cmd(0xb0 + page_pos);
		}
		write_spi_lcd_cmd(0x10 + (((col_pos) & 0xf0) >> 4));		
		write_spi_lcd_cmd(0x00 + ((col_pos) & 0x0f));
	}
		
	return count;
}

long oled_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned int page = (unsigned int)arg;
	
	switch(cmd)
	{
		case OLED_CLEAR_ALL:
		{
			clear_screen();
			break;
		}
		case OLED_CLEAR_PAGE:
		{
			clear_pag(page);
			break;
		}
		case OLED_SET_POS:
		{
			oled_set_pos(page & 0xff, (page>>8) & 0xff);
			break;
		}
	}
	return 0;
}

static struct file_operations oled_ops = {
	.owner 			= THIS_MODULE,
	.unlocked_ioctl = oled_ioctl,
	.write 			= oled_write,
};

static int spi_oled_probe(struct spi_device *spi)
{
	oled_platform_data = (struct spi_oled_platform_data *)spi->dev.platform_data;
	oled_spi_device	   = spi;

	oled_buff = kmalloc(4096, GFP_KERNEL);
	/* 分配设备号，创建设备节点 */
	major = register_chrdev(0, "oled", &oled_ops);
	oled_cls = class_create(THIS_MODULE, "oled");
	device_create(oled_cls, NULL, MKDEV(major, 0), NULL, "oled");

	st7567_init();
	clear_screen();
	
	return 0;
}

static int spi_oled_remove(struct spi_device *spi)
{
	device_destroy(oled_cls, MKDEV(major, 0));
	class_destroy(oled_cls);
	unregister_chrdev(major, "oled");
	gpio_free(oled_platform_data->oled_dc);
	
	return 0;
}

static struct spi_driver spi_oled_driver = {
	.driver = {
		.name		= "oled",
		.owner		= THIS_MODULE,
	},
	.probe		= spi_oled_probe,
	.remove		= __devexit_p(spi_oled_remove),
};


static int spi_oled_drv_init(void)
{
	int ret;

	/* 注册spi设备驱动 */
	ret = spi_register_driver(&spi_oled_driver);
	if (ret < 0)
	{
		printk("spi_register_driver fail!\n");
		return -EBUSY;
	}
		
	return 0;
}

static void spi_oled_drv_exit(void)
{
	spi_unregister_driver(&spi_oled_driver);
}

module_init(spi_oled_drv_init);
module_exit(spi_oled_drv_exit);

MODULE_LICENSE("GPL");


