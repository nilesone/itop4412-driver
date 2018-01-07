#include <linux/module.h>
#include <linux/version.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/io.h>

static struct adc_regs{
    unsigned int ADCCON[2];
    unsigned int ADCDLY;
    unsigned int ADCDAT[3];
    unsigned int CLRINTADC;
    unsigned int ADCMUX;

};
static volatile struct adc_regs *adc_reg;
static unsigned int major;
static struct class *adc_class;

static void adc_init(void)
{
    adc_reg->ADCCON[0] = (0<<16) | (1<<14) | (0xff<<6) | (0<<2);
    adc_reg->ADCMUX    = (0<<0); 
}
static void adc_start(void)
{
    adc_reg->ADCCON[0] |= (1<<0);
}


static int adc_read_value(void)
{
    int value;
    while(1)
    {
        if(adc_reg->ADCCON[0] & (1<<15))
        {
            value = adc_reg->ADCDAT[0] & 0x3ff;
            adc_reg->ADCCON[0] &= ~(1<<0);
            return value;
        }

    }
}


static int adc_open(struct inode * inode, struct file * file)
{
    printk("Open adc success!\n");
    return 0;
}

static ssize_t adc_read(struct file *file, char __user *buff, size_t size, loff_t *ppos)
{
    int value;
    adc_start();
    value = adc_read_value();
    copy_to_user(buff, &value, sizeof(value));
    return sizeof(value);
}
static int adc_close(struct inode *inode, struct file *file)
{
    printk("Adc close!\n");
    return 0;
}


static struct file_operations adc_ops = {

    .owner      = THIS_MODULE,
    .open       = adc_open,
    .read       = adc_read,
    .release    = adc_close,
};
static int adc_drv_init(void)
{
    
    adc_reg = ioremap(0x126C0000, 64);
    
    adc_init();
    major = register_chrdev(0, "adc", &adc_ops);
    adc_class = class_create(THIS_MODULE, "adc_class");
    device_create(adc_class, NULL, MKDEV(major,0), NULL, "myadc");
    return 0;
}
static void adc_drv_exit(void)
{
    device_destroy(adc_class, MKDEV(major,0));
    class_destroy(adc_class);
    unregister_chrdev(major, "adc");
    iounmap(adc_reg);
    
}

module_init(adc_drv_init);
module_exit(adc_drv_exit);
MODULE_LICENSE("GPL");

