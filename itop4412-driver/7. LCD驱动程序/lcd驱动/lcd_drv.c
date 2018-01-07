#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/ctype.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/memory.h>
#include <linux/pm_runtime.h>
#include "s3cfb.h"
#include <linux/regulator/driver.h>	
#include <linux/delay.h>

#include <linux/wakelock.h>
#include <linux/earlysuspend.h>
#include <linux/suspend.h>


#include <mach/media.h>
#include <mach/gpio.h>
#include <plat/media.h>
#include <plat/clock.h>
#include <plat/gpio-cfg.h>
#include <plat/cpu.h>


#include "lcd_drv.h"

static int s5p_lcd_fb_setcolreg(unsigned int regno, unsigned int red,
			     unsigned int green, unsigned int blue,
			     unsigned int transp, struct fb_info *info);


static struct fb_info *s5p_lcd_fb;
static volatile unsigned int * __iomem lcd_power0;
static volatile unsigned int * __iomem lcd_power1;
static volatile unsigned int * __iomem mylcd;
static volatile unsigned int * __iomem mypwm;
static volatile unsigned int * __iomem mygpio;
static volatile unsigned int * __iomem mygpl;
static volatile unsigned int * __iomem myclk;
static volatile unsigned int * __iomem myclkblk;


static unsigned int pseudo_palette[16];

static struct fb_ops s5p_lcd_fb_ops = {
		.owner 	 = THIS_MODULE,
		.fb_setcolreg	 = s5p_lcd_fb_setcolreg,
		.fb_fillrect	 = cfb_fillrect,
		.fb_copyarea	 = cfb_copyarea,
		.fb_imageblit	 = cfb_imageblit,
};

static inline unsigned int chan_to_field(unsigned int chan, struct fb_bitfield *bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf->length;
	return chan << bf->offset;
}


static int s5p_lcd_fb_setcolreg(unsigned int regno, unsigned int red,
			     unsigned int green, unsigned int blue,
			     unsigned int transp, struct fb_info *info)
{
	unsigned int val;
	
	if (regno > 16)
		return 1;
	
	/* 用red,green,blue三原色构造出val */
	val  = chan_to_field(red,	&info->var.red);
	val |= chan_to_field(green, &info->var.green);
	val |= chan_to_field(blue,	&info->var.blue);
	
	pseudo_palette[regno] = val;
	return 0;
}

static void fb_fill_var(void)
{
	s5p_lcd_fb->var.activate		= FB_ACTIVATE_NOW;
	s5p_lcd_fb->var.vmode			= FB_VMODE_NONINTERLACED;
	s5p_lcd_fb->var.bits_per_pixel  = 32;
	s5p_lcd_fb->var.xres_virtual 	= 800;
	s5p_lcd_fb->var.yres_virtual 	= 1280;
	s5p_lcd_fb->var.xres 			= 800;
	s5p_lcd_fb->var.yres 			= 1280;
	s5p_lcd_fb->var.xoffset 		= 0;
	s5p_lcd_fb->var.yoffset 		= 0;
    s5p_lcd_fb->var.width 		= 0;
	s5p_lcd_fb->var.height 		= 0;
/* always ensure these are zero, for drop through cases below */
	s5p_lcd_fb->var.transp.offset 	= 24;
	s5p_lcd_fb->var.transp.length 	= 8;
 	s5p_lcd_fb->var.red.offset      = 16;
 	s5p_lcd_fb->var.red.length      = 8;
 	s5p_lcd_fb->var.green.offset    = 8;
 	s5p_lcd_fb->var.green.length    = 8;
 	s5p_lcd_fb->var.blue.offset     = 0;
 	s5p_lcd_fb->var.blue.length     = 8;

}

static void fb_fill_fix(void)
{
	dma_addr_t map_dma;
	s5p_lcd_fb->fix.type 	= FB_TYPE_PACKED_PIXELS;
	s5p_lcd_fb->fix.accel 	= FB_ACCEL_NONE;
	s5p_lcd_fb->fix.visual 	= FB_VISUAL_TRUECOLOR;

	s5p_lcd_fb->fix.line_length = (s5p_lcd_fb->var.xres_virtual * s5p_lcd_fb->var.bits_per_pixel)/8;
	s5p_lcd_fb->fix.smem_len 	= s5p_lcd_fb->fix.line_length * s5p_lcd_fb->var.yres_virtual;
	s5p_lcd_fb->fix.xpanstep 	= 0;
	s5p_lcd_fb->fix.ypanstep 	= 0;
	/* 分配显存，并显存首地址给screen_base */
	s5p_lcd_fb->screen_base = dma_alloc_writecombine(NULL, s5p_lcd_fb->fix.smem_len, &map_dma, GFP_KERNEL);
	s5p_lcd_fb->fix.smem_start = map_dma;
	memset(s5p_lcd_fb->screen_base, 0x88, s5p_lcd_fb->fix.smem_len);
	
}

static void s5p_set_fb_info(void)
{
	/* fb->var init */
	fb_fill_var();
	/* fb->fix init */
	fb_fill_fix();
	s5p_lcd_fb->fbops 			= &s5p_lcd_fb_ops;
	s5p_lcd_fb->flags 			= FBINFO_FLAG_DEFAULT;
	s5p_lcd_fb->pseudo_palette  = pseudo_palette;
}

static void s5p_set_pin(void)
{
	int err;
//关闭LVDS_PWDN
        writel((readl(mygpl + M_GPL1_0) & (~(0xF<<0))) | 1, mygpl + M_GPL1_0);
        writel(0<<0, mygpl + M_GPL1_0 + 1);

//开启LCD电源
    writel((readl(mygpl + M_GPL0_4) & (~(0xF<<16))) | 1<<16, mygpl + M_GPL0_4);
    writel(1<<4, mygpl + M_GPL0_4 + 1);
    msleep(100);



// gpf0,gpf1,gpf2,gpf3设置为lcd输出模式 */
 	writel(GPF0_LCD, mygpio + M_GPF0CON);
	writel(GPF1_LCD, mygpio + M_GPF1CON);
 	writel(GPF2_LCD, mygpio + M_GPF2CON);
	writel((readl(mygpio + M_GPF3CON) & (~(0xFFFF))) | GPF3_LCD, mygpio + M_GPF3CON);

//开启LVDS_PWDN
    writel((readl(mygpl + M_GPL1_0) & (~(0xF<<0))) | 1, mygpl + M_GPL1_0);
    writel(1<<0, mygpl + M_GPL1_0 + 1);
	
}

static void s5p_set_clk(void)
{
    //开启LCD控制器的电源
    writel(7<<0, lcd_power1 + M_LCD0_CONFIGURATION);    
    //CLK_GATE_IP_LCD  开启LCD模块(否则不能操控LCD控制器)
    writel(readl(myclk + M_CLK_GATE_IP_LCD) | (1<<0), myclk + M_CLK_GATE_IP_LCD);      
	//选择mpll作为fimd时钟源
	writel(readl(myclk + M_CLK_SRC_LCD0) & (~(0xF<<0)) | 0x6, myclk + M_CLK_SRC_LCD0);
	//分频设置为1/1
	writel(readl(myclk + M_CLK_DIV_LCD) & (~(0xF<<0)), myclk + M_CLK_DIV_LCD);
	//设置为RGB接口,FIMD Bypass
	writel(readl(myclkblk + M_LCDBLK_CFG) | (1<<1), myclkblk + M_LCDBLK_CFG);
    //LCDBLK_CFG2输出pwm使能
    writel(readl(myclkblk + M_LCDBLK_CFG2) | (1<<0),myclkblk + M_LCDBLK_CFG2);

}

static void s5p_set_pwm(void)
{
	
/*占空比控制背光亮度*/
	//预分频	[7:0]	数值：1-255 for timer0 and timer1
 	writel((readl(mypwm + M_TCFG0) & (~0xFF)) | 0x3F, mypwm + M_TCFG0);
	//分频	[7:4]	数值：1/1, 1/2, 1/4, 1/8, 1/16 for timer1
 	writel((readl(mypwm + M_TCFG1) & (~(0xF<<4))) | 0x4<<4, mypwm + M_TCFG1);
	//设置占空比
 	writel(300,mypwm + M_TCNTB1);
 	writel(150,mypwm + M_TCMPB1);
	//更新TCNTB1和TCMPB1
 	writel((readl(mypwm + M_TCON) & (~(0xF<<8))) | (0x1<<9), mypwm + M_TCON);
}


static void s5p_set_lcd(void)
{
    unsigned int buff,i;	
//设置输出模式VIDCON0
    writel(readl(mylcd + M_VIDCON0) & (~(7<<26)), mylcd + M_VIDCON0);
//设置RGB模式
    writel(readl(mylcd + M_VIDCON2) & (~((1<<15) | (1<<14) | (3<<12))),mylcd + M_VIDCON2);
//设置显示模式VIDCON0
    writel(readl(mylcd + M_VIDCON0) & (~(7<<17)), mylcd + M_VIDCON0);
//设置极性VIDCON1
    writel(readl(mylcd + M_VIDCON1) & (~(3<<9)) | (1<<9) | (1<<7), mylcd + M_VIDCON1);
//设置时序VIDTCON0和VIDTCON1
    writel(((0-1)<<24) | (8<<16) | (0<<8) | (12<<0), mylcd + M_VIDTCON0);
    writel(((0-1)<<24) | (35<<16) | (0<<8) | (45<<0), mylcd + M_VIDTCON1);
//设置lcd的分辨率VIDTCON2
    writel(M_HOZVAL | M_LINEVAL, mylcd + M_VIDTCON2);
//发现内核原本VCLK的值为51.2MHZ,取整50MHZ,VCLK = FIMD*SCLK/(CLKVAL+1),所以CLKVAL=15
    writel(readl(mylcd + M_VIDCON0) & (~((1<<16) | (1<<5) | (0xff<<6))) | (15<<6), mylcd + M_VIDCON0);
//VIDCON2
	//This bit should be set to 1
	writel((1<<14), mylcd + M_VIDCON2);
//WINCON0
//win0设置32bpp,使能窗口1       
    writel((1<<6) | (0xD<<2) | (1<<1) | (0<<0), mylcd + M_WINCON2);		//关闭窗口3
    writel((1<<6) | (0xD<<2) | (1<<1) | (1<<0) | (1<<15), mylcd + M_WINCON0);     //      
//VIDOSD0A
//左上角坐标
	writel((0<<11) | 0, mylcd + M_VIDOSD0A);
//VIDOSD0B
	//右下角坐标
	writel((799<<11) | (1279<<0), mylcd + M_VIDOSD0B);               //和上面相反的
//VIDOSD0C
	//总大小 
	writel(800*1280, mylcd + M_VIDOSD0C);
	/*帧缓冲起始地址*/
    buff = s5p_lcd_fb->fix.smem_start;
	writel(buff<<0, mylcd + M_VIDW00ADD0B0);
	/*帧缓冲结束地址*/
    buff = s5p_lcd_fb->fix.smem_start + s5p_lcd_fb->fix.smem_len;
	writel(buff<<0, mylcd + M_VIDW00ADD1B0);
	/*设置偏移和宽度*/
	writel((0<<13) | (800*4), mylcd + M_VIDW00ADD2);
	/*绑定通道1和窗口1*/
    writel(readl(mylcd + M_WINCHMAP2) & (~((7<<16) | (7<<0))) | (1<<16) | (1<<0), mylcd + M_WINCHMAP2);
	//使能通道1
	writel(1, mylcd + M_SHADOWCON);                                 //原本0x4,内核自带LCD驱动是窗口三
}

static void s5p_lcd_on()
{
	int err;	
	
/* 开启lcd控制器输出 */
	msleep(1);
	writel(readl(mylcd + M_VIDCON0) | 0x3, mylcd + M_VIDCON0);	
	msleep(250);
    
/* gpd0_1设置为pwm输出模式 */
	writel((readl(mygpio + M_GPD0CON) & (~(0xF<<4))) | GPD01_PWM, mygpio + M_GPD0CON);
//开启Timer1    
 	writel((readl(mypwm + M_TCON) & (~(0x1<<9))) | (0x1<<8), mypwm + M_TCON);
	msleep(5);
	
}
static void myioremap()
{
    lcd_power0=ioremap(M_LCDPOWER_BASE, 512);
    lcd_power1=ioremap(M_LCDPOWER2_BASE, 16);
    mygpio    = ioremap(M_GPIOBASE, 512);
	mygpl     = ioremap(M_GPL, 64);
    myclk 	  = ioremap(M_CLKBASE, 2048);
	myclkblk  = ioremap(M_LCDBLKBASE, 1024);
    mylcd     = ioremap(M_LCDBASE, 1024); 
    mypwm     = ioremap(M_PWMBASE, 128);

}
static int s5p_lcd_init(void)
{
	int ret; 
    /* ioremap */
    myioremap();
    /*申请fb_info*/	
	s5p_lcd_fb = framebuffer_alloc(0, NULL);
	/*初始化fb_info*/
	s5p_set_fb_info();   
	/*pin脚设置*/
	s5p_set_pin();
	/*时钟设置*/
	s5p_set_clk();
    mdelay(10);
	/*控制芯片设置*/
    s5p_set_lcd();                                             
	s5p_set_pwm();
	/* 开启LCD */
	s5p_lcd_on(); 
	/*注册*/
	ret = register_framebuffer(s5p_lcd_fb);
	if(ret < 0)
	{
		printk("register framebuffer fail!\n");
	}
	return 0;
}

static void s5p_lcd_exit(void)
{
	unregister_framebuffer(s5p_lcd_fb);
	//关闭lcd控制器
	writel(readl(mylcd + M_VIDCON0) & (~(0x3<<0)), mylcd + M_VIDCON0);
	dma_free_writecombine(NULL, s5p_lcd_fb->fix.smem_len, s5p_lcd_fb->screen_base, s5p_lcd_fb->fix.smem_start);
	//关闭背光
	writel(readl(mypwm + M_TCON) & (~(0x1<<8)), mypwm + M_TCON);
	//关闭lvds
	writel(0<<0, mygpl + M_GPL1_0 + 1);
	mdelay(100);
	//关闭lcd电源
	writel(0<<4, mygpl + M_GPL0_4 + 1);
	iounmap(mylcd);
	iounmap(mypwm);
	iounmap(myclkblk);
	iounmap(myclk);
	iounmap(mygpl);
	iounmap(mygpio);
    iounmap(lcd_power0);
    iounmap(lcd_power1);
	
}


module_init(s5p_lcd_init);
module_exit(s5p_lcd_exit);
MODULE_LICENSE("GPL");


