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

#include <plat/s3c64xx-spi.h>
#include <linux/spi/spi.h>

struct spi_oled_platform_data {
	unsigned int oled_dc;
	unsigned int rst;
};

static struct spi_oled_platform_data oled_platform_data = {
	.oled_dc = EXYNOS4_GPA1(5),
	.rst	 = EXYNOS4_GPK1(0),
};

static struct s3c64xx_spi_csinfo spi2_oled_csi = {
	.line 	   = EXYNOS4_GPB(4),
	.set_level = gpio_set_value,
};

static struct spi_board_info spi2_board_info[]  = {
	{
			.modalias 		 = "oled",
			.max_speed_hz 	 = 10*1000*1000,
			.bus_num 		 = 2,
			.mode   		 = SPI_MODE_3,
			.controller_data = &spi2_oled_csi,
			.platform_data   = &oled_platform_data,
	},
	{	},

};

static int spi_oled_board_info(void)
{
	gpio_request(EXYNOS4_GPB(4), "st7567_CS");	   

	s3c_gpio_cfgpin(EXYNOS4_GPB(4), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(EXYNOS4_GPB(4), S3C_GPIO_PULL_UP);
	s5p_gpio_set_drvstr(EXYNOS4_GPB(4), S5P_GPIO_DRVSTR_LV4);
	gpio_set_value(EXYNOS4_GPB(4), 1);
	
	spi_register_board_info(spi2_board_info, ARRAY_SIZE(spi2_board_info));
	return 0;
}

module_init(spi_oled_board_info);

MODULE_LICENSE("GPL");


