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

#include <linux/clk.h>
#include <linux/clkdev.h>

#define SCL		EXYNOS4_GPC1(1)
#define SDAOUT	EXYNOS4_GPC1(4) 
#define SDAIN	EXYNOS4_GPC1(3) 

static volatile void *spi2_regs;
static volatile void *CLK_SRC_PERIL1;
static volatile void *CLK_DIV_PERIL2;

static int spi_gpio_init(void)
{
	int ret;

	ret = gpio_request(SCL, "SPI_SCL");       
	if(ret < 0)
	{
		printk("gpio_request SCL fail!\n");
		return -1;
	}
	s3c_gpio_cfgpin(SCL, S3C_GPIO_SFN(0x5));

	
	ret = gpio_request(SDAOUT, "SPI_SDAOUT");       
	if(ret < 0)
	{
		printk("gpio_request SDAOUT fail!\n");
		return -1;
	}
	s3c_gpio_cfgpin(SDAOUT, S3C_GPIO_SFN(0x5));

	ret = gpio_request(SDAIN, "SPI_SDAIN");       
	if(ret < 0)
	{
		printk("gpio_request SDAIN fail!\n");
		return -1;
	}
	s3c_gpio_cfgpin(SDAIN, S3C_GPIO_SFN(0x5));
	
	return 0;
}

static void channel_off(void)
{
	//Rx Channel关
	writel(readl(spi2_regs) & (~(1<<1)), spi2_regs + 0);
	//Tx Channel关
	writel(readl(spi2_regs) & (~(1<<0)), spi2_regs + 0);
}

static void spi_reset(void)
{
	writel(readl(spi2_regs) | (1<<5), spi2_regs + 0);
	mdelay(1);
	writel(readl(spi2_regs) & (~(1<<5)), spi2_regs + 0);
}

static void spi_controler_regs_init(void)
{
	spi_reset();		//为了方便卸载驱动后，重新装载时复位spi控制器
	/* 
	 * CH_CFGn:
	 * [3:2]  1:1   Active low : Format B
	 */
	writel((3<<2), spi2_regs + 0);
	channel_off();
}

/* 读函数因为没有对应的芯片，所以不清楚是不是正确的 */
unsigned char read_spi_byte(void)
{
	unsigned char val;
	//Rx Channel开
	writel(readl(spi2_regs) | (1<<1), spi2_regs + 0);
	/* 开始信号 */
	writel(0, spi2_regs + 0xC);
	while((readl(spi2_regs + 0x14) & (512<<15)) == 0);
	val = readl(spi2_regs + 0x1C);

	/* 结束信号 */
	writel(1, spi2_regs + 0xC);
	//Rx Channel关
	channel_off();
	return val;
}
EXPORT_SYMBOL(read_spi_byte);


void write_spi_byte(unsigned char data)
{	
	//Tx Channel开
	writel(readl(spi2_regs) | (1<<0), spi2_regs + 0);
	/* 开始信号 */
	writel(0, spi2_regs + 0xC);
	
	writel(data, spi2_regs + 0x18);
	//等待发送完成
	//while(!(readl(spi2_regs + 0x14) & (1<<25)) || (readl(spi2_regs + 0x14) & (512<<6)));
	while(readl(spi2_regs + 0x14) & (512<<6));
	
	/* 结束信号 */
	writel(1, spi2_regs + 0xC);
	//Tx Channel关
	channel_off();
}
EXPORT_SYMBOL(write_spi_byte);


static void spi_clock_init(void)
{
	/* 使能spi2控制器 */
	struct clk *spi_clk;
	spi_clk = clk_get_sys("s3c64xx-spi.2", "spi");
	clk_enable(spi_clk);
	
	/* 设置spi2的时钟源为mpll 800MHZ       */
	CLK_SRC_PERIL1 = ioremap(0x10030000 + 0xC254, 8);
	writel((readl(CLK_SRC_PERIL1) & (~(15<<24))) | (6<<24), CLK_SRC_PERIL1);
	/* 设置spi2的分频后输出频率为8MHZ 800/(9+1)/(4+1)=16 spi控制器的分频默认为2 16/2=8 */
	CLK_DIV_PERIL2 = ioremap(0x10030000 + 0xC558, 8);
	writel((9<<0) | (4<<8), CLK_DIV_PERIL2);
}

static int spi_controler_init(void)
{
	spi_clock_init();
	spi2_regs = ioremap(0x13940000, 64);
	if(spi2_regs == NULL)
	{
		printk("spi2 ioremap fail!\n");
		return -1;
	}	
	spi_gpio_init();
	spi_controler_regs_init();
		
	return 0;
}

static void spi_controler_exit(void)
{
	gpio_free(SCL);
	gpio_free(SDAIN);
	gpio_free(SDAOUT);
	iounmap(spi2_regs);
	iounmap(CLK_SRC_PERIL1);	
	iounmap(CLK_DIV_PERIL2);
}

module_init(spi_controler_init);
module_exit(spi_controler_exit);

MODULE_LICENSE("GPL");


