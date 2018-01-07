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

#define screen_max_x	800
#define screen_max_y	1280
#define FT5X0X_PT_MAX	10
#define pressure_max	255
#define FT5X0X_TS_NAME "ft5x0x_ts"

static struct ts_data{
	unsigned int x;
	unsigned int y;
	unsigned int id;
};


static unsigned int touch_point; 
static struct input_dev *ts_dev;
static struct work_struct ts_work;
static struct i2c_client *ts_client;
static struct ts_data ft5x06_data[16];
int irq;


static int ts_i2c_rxdata(char *rxdata, int length) {
	int ret;
	struct i2c_msg msgs[] = {							//第一个msg是写入要读的数据的起始地址
		{
			.addr	= ts_client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= rxdata,
		},
		{												//第二个msg是读数据
			.addr	= ts_client->addr,
			.flags	= I2C_M_RD,
			.len	= length,
			.buf	= rxdata,
		},
	};

	ret = i2c_transfer(ts_client->adapter, msgs, 2);
	if (ret < 0)
		pr_err("%s: i2c read error: %d\n", __func__, ret);

	return ret;
}


static int ts_format_data(char *buf)
{
	touch_point = buf[2] & 0xf;
	if (!touch_point) {
		input_report_abs(ts_dev, ABS_PRESSURE, 0);
		input_report_key(ts_dev, BTN_TOUCH, 0);
		input_sync(ts_dev);
		return 1;
	}
	switch(touch_point){
		case 10:
			ft5x06_data[9].x = (s16)(buf[57] & 0x0F)<<8 | (s16)buf[58];
			ft5x06_data[9].y = (s16)(buf[59] & 0x0F)<<8 | (s16)buf[60];
			ft5x06_data[9].id = buf[59]>>4;
		case 9:
			ft5x06_data[8].x = (s16)(buf[51] & 0x0F)<<8 | (s16)buf[52];
			ft5x06_data[8].y = (s16)(buf[53] & 0x0F)<<8 | (s16)buf[54];
			ft5x06_data[8].id = buf[53]>>4;
		case 8:
			ft5x06_data[7].x = (s16)(buf[45] & 0x0F)<<8 | (s16)buf[46];
			ft5x06_data[7].y = (s16)(buf[47] & 0x0F)<<8 | (s16)buf[48];
			ft5x06_data[7].id = buf[47]>>4;
		case 7:
			ft5x06_data[6].x = (s16)(buf[39] & 0x0F)<<8 | (s16)buf[40];
			ft5x06_data[6].y = (s16)(buf[41] & 0x0F)<<8 | (s16)buf[42];
			ft5x06_data[6].id = buf[41]>>4;

		case 6:
			ft5x06_data[5].x = (s16)(buf[33] & 0x0F)<<8 | (s16)buf[34];
			ft5x06_data[5].y = (s16)(buf[35] & 0x0F)<<8 | (s16)buf[36];
			ft5x06_data[5].id = buf[35]>>4;
		case 5:
			ft5x06_data[4].x = (s16)(buf[0x1b] & 0x0F)<<8 | (s16)buf[0x1c];
			ft5x06_data[4].y = (s16)(buf[0x1d] & 0x0F)<<8 | (s16)buf[0x1e];
			ft5x06_data[4].id = buf[0x1d]>>4;
		case 4:
			ft5x06_data[3].x = (s16)(buf[0x15] & 0x0F)<<8 | (s16)buf[0x16];
			ft5x06_data[3].y = (s16)(buf[0x17] & 0x0F)<<8 | (s16)buf[0x18];
			ft5x06_data[3].id = buf[0x17]>>4;
		case 3:
			ft5x06_data[2].x = (s16)(buf[0x0f] & 0x0F)<<8 | (s16)buf[0x10];
			ft5x06_data[2].y = (s16)(buf[0x11] & 0x0F)<<8 | (s16)buf[0x12];
			ft5x06_data[2].id = buf[0x11]>>4;
		case 2:
			ft5x06_data[1].x = (s16)(buf[0x09] & 0x0F)<<8 | (s16)buf[0x0a];
			ft5x06_data[1].y = (s16)(buf[0x0b] & 0x0F)<<8 | (s16)buf[0x0c];
			ft5x06_data[1].id = buf[0x0b]>>4;
		case 1:
			ft5x06_data[0].x = (s16)(buf[0x03] & 0x0F)<<8 | (s16)buf[0x04];
			ft5x06_data[0].y = (s16)(buf[0x05] & 0x0F)<<8 | (s16)buf[0x06];
			ft5x06_data[0].id = buf[0x05]>>4;
			break;
		default:
			return 0; 
	}

	return 0; 
}


static void ts_report_data()
{
//	int i;
/*
	if(!touch_point)
	{
		input_mt_sync(ts_dev);
		input_sync(ts_dev);
		return;
	}
	for(i = 0; i < touch_point; i++)
	{
		input_report_abs(ts_dev, ABS_MT_POSITION_X, ft5x06_data[i].x);
		input_report_abs(ts_dev, ABS_MT_POSITION_Y, ft5x06_data[i].y);
		input_report_abs(ts_dev, ABS_MT_PRESSURE, 200);
		input_report_abs(ts_dev, ABS_MT_TOUCH_MAJOR, 200);
		input_report_abs(ts_dev, ABS_MT_TRACKING_ID, ft5x06_data[i].id);
		input_mt_sync(ts_dev);
		printk("x = %d,y = %d,id = %d\n", ft5x06_data[i].x, ft5x06_data[i].y, ft5x06_data[i].id);
	}
*/
	input_report_abs(ts_dev, ABS_X, ft5x06_data[0].x);
	input_report_abs(ts_dev, ABS_Y, ft5x06_data[0].y);
	input_report_abs(ts_dev, ABS_PRESSURE, 200);
	input_report_key(ts_dev, BTN_TOUCH, 1);
	input_sync(ts_dev);
}

static void ts_work_func(struct work_struct *work)
{
	unsigned char buff[64] = { 0 };
	int ret;
	/* 1.获取触摸数据 */
	ts_i2c_rxdata(buff, 64);
	/* 2.格式化触摸数据 */
	ret = ts_format_data(buff);
	if(ret)
	{
		return;
	}
	/* 3.上报数据 */
	ts_report_data();
}

static irqreturn_t ts_gpio_irq(int irq, void *dev_id) 
{
	//启动工作队列，因为I2c是慢速设备，在此等待会拖慢系统
	schedule_work(&ts_work);
	return IRQ_HANDLED;
}


static int __devinit ft5x06_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	ts_client = client;
	/* 1.分配input_dev结构体 */
	ts_dev = input_allocate_device();

	/* 2.设置input_dev结构体 */
	set_bit(EV_SYN, ts_dev->evbit);		//能产生哪类事件
	set_bit(EV_ABS, ts_dev->evbit);
	set_bit(EV_KEY, ts_dev->evbit);
/*
	set_bit(ABS_MT_TRACKING_ID, ts_dev->absbit);	//能产生这类事件中的哪些事件
	set_bit(ABS_MT_POSITION_X, ts_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, ts_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, ts_dev->absbit);	
	set_bit(ABS_MT_TOUCH_MAJOR, ts_dev->absbit);


	input_set_abs_params(ts_dev, ABS_MT_POSITION_X, 0, screen_max_x, 0, 0);		//这些事件的范围
	input_set_abs_params(ts_dev, ABS_MT_POSITION_Y, 0, screen_max_y, 0, 0);
	input_set_abs_params(ts_dev, ABS_MT_TRACKING_ID, 0, FT5X0X_PT_MAX, 0, 0);
	input_set_abs_params(ts_dev, ABS_MT_WIDTH_MAJOR, 0, 200, 0, 0);
	input_set_abs_params(ts_dev, ABS_MT_TOUCH_MAJOR, 0, pressure_max, 0, 0);
*/

	set_bit(ABS_X, ts_dev->absbit);			//能产生这类事件中的哪些事件
	set_bit(ABS_Y, ts_dev->absbit);
	set_bit(ABS_PRESSURE, ts_dev->absbit);
	set_bit(BTN_TOUCH, ts_dev->keybit);

	input_set_abs_params(ts_dev, ABS_X, 0, screen_max_x, 0, 0);			//这些事件的范围
	input_set_abs_params(ts_dev, ABS_Y, 0, screen_max_y, 0, 0);
	input_set_abs_params(ts_dev, ABS_PRESSURE, 0, pressure_max, 0 , 0);


	ts_dev->name = FT5X0X_TS_NAME;

	/* 3.注册 */
	input_register_device(ts_dev);

	/* 初始化工作队列 */
	INIT_WORK(&ts_work, ts_work_func);
		
	/* 申请中断，当触摸屏被点下时，通过中断通知驱动程序去取数据 */
	gpio_request(EXYNOS4_GPX0(4), "ts_irq");       
	s3c_gpio_cfgpin(EXYNOS4_GPX0(4), S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(EXYNOS4_GPX0(4), S3C_GPIO_PULL_NONE);
	irq = gpio_to_irq(EXYNOS4_GPX0(4));
	request_irq(irq, ts_gpio_irq, IRQ_TYPE_EDGE_FALLING, "ts_irq", NULL);
	
	return 0;
}

static int __devexit ft5x06_remove(struct i2c_client *client)
{
	printk("This is ft5x06_remove!\n");
	free_irq(irq, NULL);
	gpio_free(EXYNOS4_GPX0(4));
	cancel_work_sync(&ts_work);
	input_unregister_device(ts_dev);
	input_free_device(ts_dev);
	return 0;

}



static const struct i2c_device_id ft5x06_id_table[] = {
	{"my_ft5x06", 0},
	{}
};

static struct i2c_driver ft5x06_driver = {
	.driver	= {
		.name	= "ts_ft5x06",
		.owner	= THIS_MODULE,
	},
	.probe		= ft5x06_probe,
	.remove		= __devexit_p(ft5x06_remove),
	.id_table	= ft5x06_id_table,				//跟dev中的i2c_board_info中的type比较
};
static int ft5x06_drv_init(void)
{

	i2c_add_driver(&ft5x06_driver);

	return 0;
}

static void ft5x06_drv_exit(void)
{
	i2c_del_driver(&ft5x06_driver);
}

module_init(ft5x06_drv_init);
module_exit(ft5x06_drv_exit);
MODULE_LICENSE("GPL");

