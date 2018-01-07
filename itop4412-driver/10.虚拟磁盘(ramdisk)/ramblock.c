
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/genhd.h>
#include <linux/hdreg.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/wait.h>
#include <linux/blkdev.h>
#include <linux/mutex.h>
#include <linux/blkpg.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/gfp.h>

#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/dma.h>

#define RAMBLOCK_SIZE (1024*1024)

static struct request_queue *ramblock_queue;
struct gendisk *ramblock_disk;
static DEFINE_SPINLOCK(ramblock_lock);
static unsigned char *ramblock_buff;
int major;

static int ramblock_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	geo->heads 		= 2;							//磁头
	geo->sectors 	= 32;							//扇区
	geo->cylinders  = RAMBLOCK_SIZE/2/32/512;		//柱面
	return 0;
}


static const struct block_device_operations ramblock_fops = {
	.owner	= THIS_MODULE,
	.getgeo = ramblock_getgeo,
};


static void do_ramblock_request (struct request_queue * q)
{
	static int r_cnt = 0;
	static int w_cnt = 0;
	struct request *req;

	req = blk_fetch_request(q);
	while(req)
	{
		unsigned long offset = req->__sector * 512;					//req->__sector单位是扇区
		unsigned long len	 = blk_rq_cur_bytes(req);				//blk_rq_cur_bytes(req)返回值单位是字节
		printk("do_ramblock_request offset %d\n", offset);
		printk("do_ramblock_request len %d\n", len);
		if(rq_data_dir(req) == READ)
		{
			printk("do_ramblock_request read %d\n", ++r_cnt);
			memcpy(req->buffer, ramblock_buff+offset, len);
		}
		else
		{
			printk("do_ramblock_request write %d\n", ++w_cnt);
			memcpy(ramblock_buff+offset, req->buffer, len);
			printk("Write success!\n");
		}
		if (!__blk_end_request_cur(req, 0))
			req = blk_fetch_request(q);
	}
}


static int ramblock_init(void)
{

	printk("This is ramblock!\n");
	/* 1.分配gendisk结构体 */
	ramblock_disk = alloc_disk(16);			//16表示有15个分区

	/* 2.设置 */
	/* 2.1设置ramblock 提供读写能力 */
	ramblock_queue 		 = blk_init_queue(do_ramblock_request, &ramblock_lock);
	ramblock_disk->queue = ramblock_queue;

	/* 2.2设置其他属性 */
	major = register_blkdev(0, "ramblock");			//在proc/devices中显示的名称			
	ramblock_disk->major 		= major;
	ramblock_disk->first_minor  = 0;
	ramblock_disk->fops 		= &ramblock_fops;
	sprintf(ramblock_disk->disk_name, "ramblock_disk");		//在/dev下显示的名称
	set_capacity(ramblock_disk, RAMBLOCK_SIZE / 512);		//设置扇区个数

	/* 3.硬件相关的操作 */
	ramblock_buff = kzalloc(RAMBLOCK_SIZE, GFP_KERNEL);
	
	/* 4.注册设备 */
	add_disk(ramblock_disk);
	
	return 0;
}

static void ramblock_exit(void)
{
	
	unregister_blkdev(major, "ramblock");
	del_gendisk(ramblock_disk);
	put_disk(ramblock_disk);
	blk_cleanup_queue(ramblock_queue);
	kfree(ramblock_buff);
	
}

module_init(ramblock_init);
module_exit(ramblock_exit);

MODULE_LICENSE("GPL");

