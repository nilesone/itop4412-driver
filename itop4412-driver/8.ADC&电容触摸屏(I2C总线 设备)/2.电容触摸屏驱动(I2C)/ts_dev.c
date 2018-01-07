#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/slab.h>

#include <linux/input.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <plat/gpio-cfg.h>

static const unsigned short addr_list[] = {0x70>>1, I2C_CLIENT_END};

static struct i2c_client *ft5x06_client;


static void ft5x06_init(void)
{
	/* 使能电平转换 */
	 gpio_request(EXYNOS4_GPL0(2), "TP1_EN");       
     gpio_direction_output(EXYNOS4_GPL0(2), 1);
     s3c_gpio_cfgpin(EXYNOS4_GPL0(2), S3C_GPIO_OUTPUT);
     gpio_free(EXYNOS4_GPL0(2));
     mdelay(5);
	 /* 使能触摸屏 */
     gpio_request(EXYNOS4_GPX0(3), "GPX0_3");     
     gpio_direction_output(EXYNOS4_GPX0(3), 0);
     mdelay(200);
     gpio_direction_output(EXYNOS4_GPX0(3), 1);
     s3c_gpio_cfgpin(EXYNOS4_GPX0(3), S3C_GPIO_OUTPUT);
     gpio_free(EXYNOS4_GPX0(3));
     msleep(300);
}

static int ts_dev_init(void)
{
	static struct i2c_board_info ft5x06_info;
	struct i2c_adapter *i2c_adap;
	/* 开启触摸屏电源 */
	ft5x06_init();
		
	memset(&ft5x06_info, 0, sizeof(struct i2c_board_info));	
	strlcpy(ft5x06_info.type, "my_ft5x06", I2C_NAME_SIZE);
	//获取第三个i2c适配器的信息
	i2c_adap = i2c_get_adapter(3);
	/* 遍历adapter(3)中addr_list里的每个设备地址,找到有回应的调用i2c_new_device */
	ft5x06_client = i2c_new_probed_device(i2c_adap, &ft5x06_info, addr_list, NULL);
	if(!ft5x06_client)
	{
		printk("Add device fail!\n");
	}
	
	/* 释放adapter */
	i2c_put_adapter(i2c_adap);
	
	return 0;
}

static void ts_dev_exit(void)
{
	gpio_free(EXYNOS4_GPL0(2));
	gpio_free(EXYNOS4_GPX0(3));
	i2c_unregister_device(ft5x06_client);
}


module_init(ts_dev_init);
module_exit(ts_dev_exit);
MODULE_LICENSE("GPL");

