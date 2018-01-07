#include <linux/init.h>
#include <linux/module.h>

#include <linux/kernel.h>
#include <linux/fs.h>

#include <linux/platform_device.h>

#include <mach/regs-gpio.h>
#include <asm/io.h>
#include <linux/regulator/consumer.h>

#include <linux/delay.h>

//copy_to_user的头文件
#include <asm/uaccess.h>

#include <linux/irq.h>
#include <linux/interrupt.h>

#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/dma-mapping.h>
#include <mach/dma.h>
#include <plat/s3c-dma-pl330.h>


#define MEM_CPY_NO_DMA	0
#define MEM_CPY_DMA		1
#define BUFF_SIZE		(512*1024)

static int major = 0;
static struct class *cls;

static char *src;
static dma_addr_t src_phys;
static char *dst;
static dma_addr_t dst_phys;

static int ev_dma;

unsigned int dma_id;

static struct s3c2410_dma_client dma_cl = { 
	.name = "dma_test",
};

static int dma_thread;


static DECLARE_WAIT_QUEUE_HEAD(dma_waitq);

static long dma_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int i;

	memset(src, 0xAA, BUFF_SIZE);
	memset(dst, 0xBB, BUFF_SIZE);
	
	if(memcmp(src, dst, BUFF_SIZE) != 0)
	{
		printk("dmabuff init finish!\n");
	}
	else
	{
		printk("dmabuff init fail!\n");
	}

	
	if (cmd == MEM_CPY_NO_DMA)
	{
		for(i = 0; i < BUFF_SIZE; i++)
		{
			dst[i] = src[i];
		}
		if(memcmp(src, dst, BUFF_SIZE) == 0)
		{
			printk("MEM_CPY_NO_DMA OK!\n");
		}
		else
		{
			printk("MEM_CPY_NO_DMA FAIL!\n");
		}
	}
	else
	{
		ev_dma = 0;
		
		/* 5.设置目的地址和大小并把dma传输放如队列 5和6要放一起 */
		s3c2410_dma_enqueue(DMACH_MTOM_0 + dma_id, (void *)(&dma_thread), dst_phys, BUFF_SIZE);
		/* 6.开启dma传输 */
		s3c2410_dma_ctrl(DMACH_MTOM_0 + dma_id, S3C2410_DMAOP_START);
		
		wait_event_interruptible(dma_waitq, ev_dma);
		
		if(memcmp(src, dst, BUFF_SIZE) == 0)
		{
			printk("MEM_CPY_DMA OK!\n");
		}
		else
		{
			printk("MEM_CPY_DMA FAIL!\n");
		}
	}
	
	return 0;
}

static struct file_operations dma_fops = {
	.owner  		 = THIS_MODULE,
	.unlocked_ioctl  = dma_ioctl,
};

static void dma_callback_func(void *dma_async_param)
{
	ev_dma = 1;
	wake_up_interruptible(&dma_waitq);	 /* 唤醒休眠的进程 */
}

static int dma_init(void)
{
	int ret;

	/* 分配一个物理上连续的内存 */
	src = dma_alloc_writecombine(NULL, BUFF_SIZE, &src_phys, GFP_KERNEL);
	if (NULL == src)
	{
		dma_free_writecombine(NULL, BUFF_SIZE, src, src_phys);
		printk("alloc src_dma faile!\n");
		return -ENOMEM;
	}
	dst = dma_alloc_writecombine(NULL, BUFF_SIZE, &dst_phys, GFP_KERNEL);
	if (NULL == dst)
	{
		dma_free_writecombine(NULL, BUFF_SIZE, dst, dst_phys);
		printk("alloc dst_dma faile!\n");
		return -ENOMEM;
	}

	/* 1.申请一个通道 */
	ret = s3c2410_dma_request(DMACH_MTOM_0 + dma_id, &dma_cl, NULL);
	if (ret) {
		printk("request channel fail!\n");
		return -ENOMEM;
	}
	/* 2.设置回调函数 */
	s3c2410_dma_set_buffdone_fn(DMACH_MTOM_0 + dma_id, dma_callback_func);
	/* 3.初始化通道 */
	ret = s3c2410_dma_config(DMACH_MTOM_0 + dma_id, 1);
	if (ret) {
		printk("config channel fail!\n");
		return -ENOMEM;
	}
	/* 4.设置源地址和传输类型 */
	s3c2410_dma_devconfig(DMACH_MTOM_0 + dma_id, S3C_DMA_MEM2MEM, src_phys);

	major = register_chrdev(0, "dma", &dma_fops);
	cls = class_create(THIS_MODULE, "dma_cls");
	device_create(cls, NULL, MKDEV(major, 0), NULL, "dma_test");
	
	return 0;
}

static void dma_exit(void)
{
	device_destroy(cls, MKDEV(major, 0));
	class_destroy(cls);
	unregister_chrdev(major, "dma");
	s3c2410_dma_free(DMACH_MTOM_0 + dma_id, &dma_cl);
	dma_free_writecombine(NULL, BUFF_SIZE, dst, dst_phys);
	dma_free_writecombine(NULL, BUFF_SIZE, src, src_phys);
}

module_init(dma_init);
module_exit(dma_exit);

MODULE_LICENSE("GPL");



