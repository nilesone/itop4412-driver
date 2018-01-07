#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <plat/iic.h>
#include <plat/gpio-cfg.h>

#include <linux/clk.h>
#include <linux/clkdev.h>

#include <asm/irq.h>

#include <plat/regs-iic.h>
#include <plat/iic.h>

#define I2CCON	0x0000
#define I2CSTAT	0x0004
#define I2CADD	0x0008
#define I2CDS	0x000C
#define I2CLC	0x0010



static void __iomem *i2c_regs;
enum i2c_state {
	STATE_IDLE,
	STATE_START,
	STATE_READ,
	STATE_WRITE,
	STATE_STOP,
};
struct i2c_xfer_data {
	struct i2c_msg *msg;
	int msg_num;
	int msg_cur;
	int ptr_cur;
	int state;	
	int err;
	wait_queue_head_t wait;
};

static struct i2c_xfer_data i2c_xfer_data;


static void exynos_i2c_start(void)
{
	i2c_xfer_data.state = STATE_START;
	if(i2c_xfer_data.msg->flags & I2C_M_RD)
	{
		writel(i2c_xfer_data.msg->addr<<1 | 1, i2c_regs + I2CDS);		//读
		ndelay(50);
		writel(0xb0, i2c_regs + I2CSTAT);
	}
	else
	{
		writel(i2c_xfer_data.msg->addr<<1, i2c_regs + I2CDS);			//写
		ndelay(50);
		writel(0xf0, i2c_regs + I2CSTAT);
	}
}

static void exynos_i2c_stop(int err)
{
	i2c_xfer_data.state = STATE_STOP;
	i2c_xfer_data.err	= err;
	if(i2c_xfer_data.msg->flags & I2C_M_RD)
	{
		writel(0x90, i2c_regs + I2CSTAT);
		writel(0xaf, i2c_regs +I2CCON);
		
	}
	else
	{
		writel(0xd0, i2c_regs + I2CSTAT);
		writel(0xaf, i2c_regs +I2CCON);
		
	}
	wake_up(&i2c_xfer_data.wait);
}

static int exynos4412_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	int time_out;
	i2c_xfer_data.msg = msgs;
	i2c_xfer_data.msg_num = num;
	i2c_xfer_data.msg_cur = 0;
	i2c_xfer_data.ptr_cur = 0;
	i2c_xfer_data.err = -ENODEV;

	exynos_i2c_start();
	time_out = wait_event_timeout(i2c_xfer_data.wait, i2c_xfer_data.state == STATE_STOP, HZ * 5);
	if(time_out == 0)
	{
		printk("Read/write time out!\n");
		return -ETIMEDOUT;
	}
	else
	{
		return i2c_xfer_data.err;
	}

}

static u32 exynos4412_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_PROTOCOL_MANGLING;
}

static int islastmsg(void)
{
	return (i2c_xfer_data.msg_cur == (i2c_xfer_data.msg_num - 1));
}

static int isEndData(void)
{
	return (i2c_xfer_data.ptr_cur >= i2c_xfer_data.msg->len);
}

static int islastData(void)
{
	return (i2c_xfer_data.ptr_cur >= (i2c_xfer_data.msg->len - 1));
}

static irqreturn_t exynos4412_i2c_xfer_irq(int irq, void *dev_id)
{
	int stat = 0;
	
	stat = readl(i2c_regs + I2CSTAT);
	if(stat & 0x8) printk("Bus arbitration fails during serial I/O\n");
	
	switch(i2c_xfer_data.state)
	{
		case STATE_START:
			{
				if(stat & 0x1)
				{
					printk("Dose not received ACK!\n");
					exynos_i2c_stop(-ENODEV);
					break;
				}
				if(i2c_xfer_data.msg->flags & I2C_M_RD)
				{
					i2c_xfer_data.state = STATE_READ;
					goto next_read;
				}
				else
				{
					i2c_xfer_data.state = STATE_WRITE;
				}
			}
		case STATE_WRITE:
			{	
				if(stat & 0x1)
				{
					exynos_i2c_stop(-ENODEV);
					break;
				}
				if(!isEndData())			//当前msg中将要发送的数据还没完
				{
					writeb(i2c_xfer_data.msg->buf[i2c_xfer_data.ptr_cur], i2c_regs + I2CDS);
					i2c_xfer_data.ptr_cur++;
					ndelay(50);
					break;
				}
				if(!islastmsg())			//当前msg不是最后一个msg
				{
					i2c_xfer_data.msg++;
					i2c_xfer_data.msg_cur++;
					i2c_xfer_data.ptr_cur = 0;
					i2c_xfer_data.state = STATE_START;
					exynos_i2c_start();
					break;
				}
				else					//所有msg数据中的数据都已经发送完毕
				{
					exynos_i2c_stop(0);
					break;
				}
			}	
		case STATE_READ:
			{
				i2c_xfer_data.msg->buf[i2c_xfer_data.ptr_cur] = readb(i2c_regs + I2CDS);
				i2c_xfer_data.ptr_cur++;
next_read:		
				if(!isEndData())				//当前msg中将要接受的数据还没完
				{
					if(islastData())						//将要发送的数据是最后一个，不发送ack信号
					{
						writel(0x2f, i2c_regs + I2CCON);
					}
					else
					{
						writel(0xaf, i2c_regs + I2CCON);
					}
					break;
				}
				if(!islastmsg())				//当前msg不是最后一个msg
				{
					i2c_xfer_data.msg++;
					i2c_xfer_data.msg_cur++;
					i2c_xfer_data.ptr_cur = 0;
					i2c_xfer_data.state = STATE_START;
					exynos_i2c_start();
					break;
				}
				else
				{
					exynos_i2c_stop(0);
					break;
				}
			}
	}
		
		/* 清除中断 */
	writel(readl(i2c_regs + I2CCON) & (~(1<<4)), i2c_regs + I2CCON);
		
	return IRQ_HANDLED;	
}

static void exynos_i2c_init(void)
{
	struct clk *clk;
	/* 使能I2c控制器 */
	clk = clk_get_sys("s3c2440-i2c.3", "i2c");
	clk_enable(clk);
	/* 设置引脚为I2c方式 */
	s3c_gpio_setpull(EXYNOS4_GPA1(2), S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(EXYNOS4_GPA1(2), S3C_GPIO_SFN(3));
	s3c_gpio_setpull(EXYNOS4_GPA1(3), S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(EXYNOS4_GPA1(3), S3C_GPIO_SFN(3));
	/* ack和中断使能 */
	writel(1<<7 | 1<<5, i2c_regs + I2CCON);
	/* clock源和分频 */
	writel(readl(i2c_regs + I2CCON) & (~(0xf | 1<<6)) | 0xf, i2c_regs + I2CCON);
	/* slave address设置 */
	writel(0x10, i2c_regs + I2CADD);
	/* 使能输出 */
	writel(1<<4, i2c_regs + I2CSTAT);
}

static struct i2c_algorithm exynos4412_i2c_algo = {
	.master_xfer 	= exynos4412_i2c_xfer,
	.functionality  = exynos4412_i2c_func,
};
	
	/* 1.分配设置adapter结构体 */
static struct i2c_adapter exynos4412_i2c_adapter = {
	.name 	= "exynos4412",
	.algo 	= &exynos4412_i2c_algo,
	.owner 	= THIS_MODULE,
};

static int i2c_adap_init(void)
{
	/* i2c相关寄存器 */
	i2c_regs = ioremap(0x13890000, 32);
	
	/* 中断申请 */
	request_irq(IRQ_IIC3, exynos4412_i2c_xfer_irq, 0, "exynos4412_i2c", NULL);
	
	/* 硬件相关的操作 */
	exynos_i2c_init();

	init_waitqueue_head(&i2c_xfer_data.wait);
	
	/* 注册I2c adapter */
	exynos4412_i2c_adapter.nr = 3;
	i2c_add_numbered_adapter(&exynos4412_i2c_adapter);
	
	return 0;
}

static void i2c_adap_exit(void)
{
	struct clk *clk;
	i2c_del_adapter(&exynos4412_i2c_adapter);
	
	/* 关闭I2c控制器 */
	clk = clk_get_sys("s3c2440-i2c.3", "i2c");
	clk_disable(clk);
	
	free_irq(IRQ_IIC3, NULL);
	iounmap(i2c_regs);
}


module_init(i2c_adap_init);
module_exit(i2c_adap_exit);
MODULE_LICENSE("GPL");


