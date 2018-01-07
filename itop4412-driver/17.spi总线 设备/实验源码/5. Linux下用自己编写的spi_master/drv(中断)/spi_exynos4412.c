#include <linux/init.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>

#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <linux/irqflags.h>

#include <linux/irq.h>
#include <linux/interrupt.h>

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/gpio-exynos4.h>
#include <mach/regs-gpio.h>

#include <linux/wait.h>
#include <linux/completion.h>
#include <mach/irqs-exynos4.h>
#include <linux/sched.h>

struct enynos4412_spi_info{
	struct platform_device *pdev;
    void __iomem *spi_regs_base;		
	void __iomem *spi_baud_regs1;
	void __iomem *spi_baud_regs2;
	unsigned int spi_clk_pin;
	unsigned int spi_mosi_pin;	
	unsigned int spi_miso_pin;
	struct work_struct spi_work;
	struct spi_device *spi_dev;
	struct spi_message *spi_mesg;
	struct spi_transfer *cur_t;

	struct completion done;
	unsigned int cur_cnt;
	
	int irq;
};

struct exynos4412_spi_csinfo {
	u8 fb_delay;
	unsigned line;
	void (*set_level)(unsigned line_id, int lvl);
};

struct spi_master *spi2_master;	

static void spi2_clock_init(struct enynos4412_spi_info *spi_info)
{
	/* 使能spi2控制器 */
	struct clk *spi_clk;
	spi_clk = clk_get_sys("s3c64xx-spi.2", "spi");
	clk_enable(spi_clk);
	
	/* 设置spi2的时钟源为mpll 800MHZ       */
	spi_info->spi_baud_regs1 = ioremap(0x10030000 + 0xC254, 8);
	writel((readl(spi_info->spi_baud_regs1) & (~(15<<24))) | (6<<24), spi_info->spi_baud_regs1);

	spi_info->spi_baud_regs2 = ioremap(0x10030000 + 0xC558, 8);

}

static int spi_gpio_init(struct enynos4412_spi_info *spi_info)
{
	int ret;

	ret = gpio_request(spi_info->spi_clk_pin, "SPI_SCL");       
	if(ret < 0)
	{
		printk("gpio_request SCL fail!\n");
		return -1;
	}
	s3c_gpio_cfgpin(spi_info->spi_clk_pin, S3C_GPIO_SFN(0x5));

	
	ret = gpio_request(spi_info->spi_mosi_pin, "SPI_SDAOUT");       
	if(ret < 0)
	{
		printk("gpio_request SDAOUT fail!\n");
		return -1;
	}
	s3c_gpio_cfgpin(spi_info->spi_mosi_pin, S3C_GPIO_SFN(0x5));

	ret = gpio_request(spi_info->spi_miso_pin, "SPI_SDAIN");       
	if(ret < 0)
	{
		printk("gpio_request SDAIN fail!\n");
		return -1;
	}
	s3c_gpio_cfgpin(spi_info->spi_miso_pin, S3C_GPIO_SFN(0x5));
	
	return 0;
}

static void channel_off(void __iomem *spi_regs)
{
	//Rx Channel关
	writel(readl(spi_regs) & (~(1<<1)), spi_regs + 0);
	//Tx Channel关
	writel(readl(spi_regs) & (~(1<<0)), spi_regs + 0);
}

static void spi_reset(void __iomem *spi_regs)
{
	writel(readl(spi_regs) | (1<<5), spi_regs + 0);
	mdelay(1);
	writel(readl(spi_regs) & (~(1<<5)), spi_regs + 0);
}

static int exynos4412_spi_setup(struct spi_device *spi)
{
	struct enynos4412_spi_info *spi_info;
	unsigned int div = 0;
	
	spi_info = spi_master_get_devdata(spi->master);
	
	/* 1.设置spi的分频后输出频率为800/(7+1)=100MHZ 
	 * 100/(x+1)=spi->max_speed_hz*2   x=100/(((spi->max_speed_hz/1000)/1000)*2)-1        
	 * spi控制器的分频默认为2 
	 */
	div = 100 / (spi->max_speed_hz / 1000 / 1000 * 2) - 1;
	if (div > 255)
		div = 255;
	writel((7<<0) | (div<<8), spi_info->spi_baud_regs2);	
	
	/* 2.设置spi控制寄存器 */
	/* 
	 * CH_CFGn:
	 * [3:2]     Active : Format 
	 */
	writel(spi->mode << 2, spi_info->spi_regs_base + 0);
	writel((1<<5) | (1<<11), spi_info->spi_regs_base + 0x8);
	channel_off(spi_info->spi_regs_base);
	
	return 0;
}

static void spi_work_func(struct work_struct *work)
{
	struct spi_transfer *xfer;
	struct exynos4412_spi_csinfo *spi_csinfo;
	struct enynos4412_spi_info *spi_info;
		
	spi_info = container_of(work, struct enynos4412_spi_info, spi_work);
	
	/* 1.使能片选 */
	spi_csinfo = (struct exynos4412_spi_csinfo *)spi_info->spi_dev->controller_data;
	spi_csinfo->set_level(spi_csinfo->line, 0);
	
	/* 2.传输数据 */
	/* 2.1 传输第一个数据之前先setup */
	spi_info->spi_dev->master->setup(spi_info->spi_dev);

	/* 2.2 遍历spi_message中的每一个spi_transfer */
	list_for_each_entry(xfer, &spi_info->spi_mesg->transfers, transfer_list) {
			
		spi_info->cur_t   = xfer;
		spi_info->cur_cnt = 0;
		init_completion(&spi_info->done);
		
		if (xfer->tx_buf)		/* 2.2.1 如果是写 */
		{
			//Tx Channel开
			writel(readl(spi_info->spi_regs_base) | (1<<0), spi_info->spi_regs_base + 0);
			/* 开始信号 */
			writel(0, spi_info->spi_regs_base + 0xC);	
			/* 发送第一个数据 */
			writeb(((unsigned char *)(xfer->tx_buf))[0], spi_info->spi_regs_base + 0x18);
			/* 使能tx完成产生中断 必须放在开始信号后面 否则系统会死机 */
			writel(readl(spi_info->spi_regs_base + 0x10) | (1<<0), spi_info->spi_regs_base + 0x10);
			//睡眠等待发送完成
			wait_for_completion(&spi_info->done);
			/* 结束信号 */
			writel(1, spi_info->spi_regs_base + 0xC);
			//Tx Channel关
			channel_off(spi_info->spi_regs_base);
		}		
		else  				/* 2.2.2 如果是读 */
		{
			//Rx Channel开
			writel(readl(spi_info->spi_regs_base) | (1<<1), spi_info->spi_regs_base + 0);
			/* 开始信号 */
			writel(0, spi_info->spi_regs_base + 0xC);
			/* 使能rx完成产生中断 必须放在开始信号后面 否则系统会死机 */
			writel(readl(spi_info->spi_regs_base + 0x10) | (1<<1), spi_info->spi_regs_base + 0x10);
			//睡眠等待接受完成
			wait_for_completion(&spi_info->done);
			/* 结束信号 */
			writel(1, spi_info->spi_regs_base + 0xC);
			//Rx Channel关
			channel_off(spi_info->spi_regs_base);
		}
	}
	
	/* 3.关闭片选 */
	spi_csinfo->set_level(spi_csinfo->line, 1);
	
	/* 4 唤醒在调用spi_transfer而spi_transfer返回0后休眠的进程 */
	spi_info->spi_mesg->status = 0;
	spi_info->spi_mesg->complete(spi_info->spi_mesg->context);  

	spi_info->cur_t = NULL;	
}

static int exynos4412_spi_transfer(struct spi_device *spi, struct spi_message *mesg)
{
	struct enynos4412_spi_info *spi_info;
	
	spi_info = spi_master_get_devdata(spi->master);
	spi_info->spi_dev = spi;
	spi_info->spi_mesg = mesg;
	/* 用工作队列的原因是无法在spi_transfer函数中休眠，会造成错误 */	
	schedule_work(&spi_info->spi_work);
	
	return 0;
}

static irqreturn_t exynos_spi_irq(int irq, void *dev_id)
{
	struct enynos4412_spi_info *spi_info;
	struct spi_master *master;
	struct spi_transfer *t;
	master = (struct spi_master *)dev_id;
	spi_info = spi_master_get_devdata(master);
	t =  spi_info->cur_t;
    if (!t)
    {
        /* 误触发 */
		/* 关中断 */
		printk("wu\n");
		writel(0, spi_info->spi_regs_base + 0x10);
		
    	return IRQ_HANDLED;            
    }
	if (t->tx_buf)						/* 2.是发送 */
	{
		spi_info->cur_cnt++;
		if (spi_info->cur_cnt < t->len)		/*没发完 */
		{
			writeb(((unsigned char *)(t->tx_buf))[spi_info->cur_cnt], spi_info->spi_regs_base + 0x18);
		}
		else
		{
			complete(&spi_info->done); 		/* 唤醒 */
			/* 关中断 */
			writel(readl(spi_info->spi_regs_base + 0x10) & ~(1<<0), spi_info->spi_regs_base + 0x10);
		}
			
	}	
	else									/* 1.是接收 */
	{
		if (spi_info->cur_cnt < t->len)		/*没接完 */
		{
			((unsigned char *)(t->rx_buf))[spi_info->cur_cnt] = readb(spi_info->spi_regs_base + 0x1C);
			spi_info->cur_cnt++;
		}
		else
		{
			complete(&spi_info->done);		/* 唤醒 */
			/* 关中断 */
			writel(readl(spi_info->spi_regs_base + 0x10) & ~(1<<1), spi_info->spi_regs_base + 0x10);
		}
	}
	
	return IRQ_HANDLED;    
}

static void exynos4412_hwinit(int bus_num, struct spi_master *master)
{
	int ret;
	struct enynos4412_spi_info *spi_info;
	spi_info = spi_master_get_devdata(master);
	
	if (bus_num == 2)
	{
		spi2_clock_init(spi_info);
		spi_info->spi_regs_base = ioremap(0x13940000, 64);
		if(spi_info->spi_regs_base == NULL)
		{
			printk("spi2 ioremap fail!\n");
			return;
		}	
		
		spi_reset(spi_info->spi_regs_base); 	//为了方便卸载驱动后，重新装载时复位spi控制器
		
		spi_info->spi_clk_pin = EXYNOS4_GPC1(1);
		spi_info->spi_mosi_pin = EXYNOS4_GPC1(4);		
		spi_info->spi_miso_pin = EXYNOS4_GPC1(3);

		spi_gpio_init(spi_info);
		
		spi_info->irq = IRQ_SPI2;
		ret = request_irq(IRQ_SPI2, exynos_spi_irq, 0, "spi2_irq", master);
		if (ret < 0)
			printk("request_irq IRQ_SPI2 fail!\n");
	}
}

static struct spi_master *create_spi_master(int bus_num)
{
	int ret;
	
	struct spi_master *master;	
	struct platform_device *pdev;
	struct enynos4412_spi_info *spi_info;
	
	/* 1.分配一个spi_master结构体 */
	pdev = platform_device_alloc("spi_dev", bus_num);
	platform_device_add(pdev);
	master = spi_alloc_master(&pdev->dev, sizeof(struct enynos4412_spi_info));
	spi_info = spi_master_get_devdata(master);
	spi_info->pdev = pdev;
	/* 2.设置spi_master结构体 */
	master->bus_num   	   = bus_num;
	master->setup 	  	   = exynos4412_spi_setup;
	master->transfer       = exynos4412_spi_transfer;
	master->mode_bits 	   = SPI_CPOL | SPI_CPHA | SPI_CS_HIGH;
	master->num_chipselect = 0xffff;
	INIT_WORK(&spi_info->spi_work, spi_work_func);

	/* 3.硬件相关的初始化 */
	exynos4412_hwinit(bus_num, master);
	
	/* 4.注册spi_master */
	ret = spi_register_master(master);
	if (ret < 0)
	{
		printk("spi_register_master fail!\n");
	}
	
	return master;
}

static void destroy_spi_master(struct spi_master *master)
{
	struct enynos4412_spi_info *spi_info;
	spi_info = spi_master_get_devdata(master);

	free_irq(spi_info->irq, master);
	
	iounmap(spi_info->spi_regs_base);
	iounmap(spi_info->spi_baud_regs1);
	iounmap(spi_info->spi_baud_regs2);

	gpio_free(spi_info->spi_clk_pin);
	gpio_free(spi_info->spi_miso_pin);	
	gpio_free(spi_info->spi_mosi_pin);	

	cancel_work_sync(&spi_info->spi_work);
	
	platform_set_drvdata(spi_info->pdev, NULL);

	platform_device_del(spi_info->pdev);
	platform_device_put(spi_info->pdev);

	device_del(&spi_info->spi_dev->dev);

	spi_unregister_master(master);	
}

static int spi_exynos4412_init(void)
{
	spi2_master = create_spi_master(2);
	
	return 0;
}

static void spi_exynos4412_exit(void)
{
	destroy_spi_master(spi2_master);
}

module_init(spi_exynos4412_init);
module_exit(spi_exynos4412_exit);

MODULE_LICENSE("GPL");


