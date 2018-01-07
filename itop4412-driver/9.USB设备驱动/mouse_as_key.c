#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb/input.h>
#include <linux/hid.h>


static struct input_dev *input_mouseaskey;
static char *usb_buff;
static struct urb *um_urb;
int len;

static void mouse_as_key_irq(struct urb *urb)
{
/*
	static int count = 0;
	int i;
	printk("data cnt %d:", ++count);
	for(i=0; i < len; i++)
	{
		printk("%02x", usb_buff[i]);
	}
	printk("\n");
	usb_submit_urb(um_urb, GFP_KERNEL);
*/

	static int pre_val = 0 ;

	if((pre_val & 1<<0) != (usb_buff[0] & 1<<0))		//键值改变才上报
	{	
		input_event(input_mouseaskey, EV_KEY, KEY_L, (usb_buff[0] & (1<<0)) ? 1 : 0);
		input_sync(input_mouseaskey);
	}
	
	if((pre_val & 1<<1) != (usb_buff[0] & 1<<1))
	{
		input_event(input_mouseaskey, EV_KEY, KEY_S, (usb_buff[0] & (1<<1)) ? 1 : 0);
		input_sync(input_mouseaskey);
	}
	
	if((pre_val & 1<<2) != (usb_buff[0] & 1<<2))
	{
		input_event(input_mouseaskey, EV_KEY, KEY_ENTER, (usb_buff[0] & (1<<2)) ? 1 : 0);
		input_sync(input_mouseaskey);
	}

	pre_val = usb_buff[0];					//保存这次的按键值

	usb_submit_urb(um_urb, GFP_KERNEL);

}

static int mouse_as_key_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	
	struct usb_device *dev = interface_to_usbdev(intf);
	struct usb_host_interface *interface;
	struct usb_endpoint_descriptor *endpoint;
	int pipe;
	dma_addr_t data_dma;
	
	/* a.分配input_dev结构体 */
	input_mouseaskey = input_allocate_device();

	/* b.设置input_dev结构体 */
	/* 能产生哪类事件 */
	set_bit(EV_KEY, input_mouseaskey->evbit);
	set_bit(EV_REP, input_mouseaskey->evbit);

	/* 能产生的这类事件中的哪些事件 */
	set_bit(KEY_L, input_mouseaskey->keybit);
	set_bit(KEY_S, input_mouseaskey->keybit);
	set_bit(KEY_ENTER, input_mouseaskey->keybit);

	/* c.注册 */
	input_register_device(input_mouseaskey);
	
	/* d.硬件的操作 */
	/* 获取数据 */
	interface = intf->altsetting;
	endpoint = &interface->endpoint[0].desc;

	/* 数据传输3要素:源，目的，长度 */
	/* 源 */
	pipe = usb_rcvintpipe(dev, endpoint->bEndpointAddress);
	/* 长度 */
	len = endpoint->wMaxPacketSize;
	/* 目的 */
	usb_buff = usb_alloc_coherent(dev, len, GFP_ATOMIC, &data_dma);

	/* d.1分配urb */
	um_urb = usb_alloc_urb(0, GFP_KERNEL);
	/* d.2填充urb */
	usb_fill_int_urb(um_urb, dev, pipe, usb_buff, len, mouse_as_key_irq, NULL, endpoint->bInterval);
	um_urb->transfer_dma = data_dma;
	um_urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
	/* d.3提交urb */
	usb_submit_urb(um_urb, GFP_KERNEL);
	
	return 0;
}

static void mouse_as_key_disconnect(struct usb_interface *intf)
{
	printk("usb device disconnect!\n");
}

static struct usb_device_id mouse_as_key_id_table [] = {
	{ USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID, USB_INTERFACE_SUBCLASS_BOOT,
		USB_INTERFACE_PROTOCOL_MOUSE) },
	{ }	/* Terminating entry */
};


static struct usb_driver usb_mouse_as_key_driver = {

	.name 		= "mouse_as_key",
	.probe 		= mouse_as_key_probe,
	.disconnect = mouse_as_key_disconnect,
	.id_table 	= mouse_as_key_id_table,
};


static int mouse_as_key_init(void)
{
	usb_register(&usb_mouse_as_key_driver);
	return 0;
}

static void mouse_as_key_exit(void)
{
	usb_deregister(&usb_mouse_as_key_driver);
}

module_init(mouse_as_key_init);
module_exit(mouse_as_key_exit);

MODULE_LICENSE("GPL");


