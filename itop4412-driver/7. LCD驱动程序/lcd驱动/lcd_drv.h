/* lcd寄存器 */
		//0x11C0_0000
#define M_LCDBASE		0x11C00000
#define M_VIDCON0		0			//0x0000
#define M_VIDCON1 		1			//0x0004
#define M_VIDCON2 		2			//0x0008
#define M_VIDCON3 		3			//0x000C
#define M_VIDTCON0 		4			//0x0010
#define M_VIDTCON1 		5			//0x0014
#define M_VIDTCON2 		6			//0x0018
#define M_VIDTCON3 		7			//0x001C
#define M_WINCON0 		8			//0x0020
#define M_WINCON1 		9			//0x0024
#define M_WINCON2 		10			//0x0028
#define M_WINCON3 		11			//0x002C
#define M_WINCON4 		12			//0x0030
#define M_SHADOWCON 	13			//0x0034       8
#define M_WINCHMAP2 	15			//0x003C
#define M_VIDOSD0A 		16			//0x0040
#define M_VIDOSD0B 		17			//0x0044
#define M_VIDOSD0C 		18			//0x0048

#define M_VIDW00ADD0B0 	40			//0x00A0		
#define M_VIDW00ADD1B0 	52			//0x00D0		
#define M_VIDW00ADD2 	64			//0x0100	

#define M_WIN0MAP	 	96			//0x0180
#define M_VIDW0ALPHA0   135         //0x021C
#define M_VIDW0ALPHA1   136         //0x0220
#define M_BLENDCON      152         //0x0260

/* 帧同步 */
#define M_VBPDE			0xFF << 24
#define M_VBPD			0xD << 16		
#define M_VFPD			0x2 << 8		
#define M_VSPW			0x4 << 0		
/* 水平同步 */
#define M_VFPDE			0xFF << 24		
#define M_HBPD			0x4F << 16		
#define M_HFPD			0x2F << 8		
#define M_HSPW			0x1F << 0		
/* 行列数 */
#define M_HOZVAL		0x31F << 0		//800-1
#define M_LINEVAL		0x4ff << 11		//1280-1



/*pwm寄存器*/
  		//0x139D_0000
#define M_PWMBASE		0x139D0000
#define M_TCFG0			0			//0x0000  
#define M_TCFG1			1			//0x0004 
#define M_TCON			2			//0x0008  
#define M_TCNTB0  		3			//0x000C
#define M_TCMPB0  		4			//0x0010
#define M_TCNTO0  		5			//0x0014
#define M_TCNTB1 		6			//0x0018
#define M_TCMPB1  		7			//0x001C
#define M_TCNTO1 		8			//0x0020
#define M_TCNTB2 		9			//0x0024
#define M_TCMPB2 		10			//0x0028
#define M_TCNTO2 		11			//0x002C
#define M_TCNTB3		12			//0x0030
#define M_TCMPB3 		13			//0x0034
#define M_TCNTO3		14			//0x0038
#define M_TCNTB4		15			//0x003C
#define M_TCNTO4		16			//0x0040
#define M_TINT_CSTAT 	17			//0x0044



/*io管脚寄存器*/
		//0x1140_0000
#define M_GPIOBASE		0x11400000
#define M_GPD0CON		40				//0x00A0	gpd0_1		
#define M_GPF0CON		96				//0x0180	gpf0_0到gpf0_7	
#define M_GPF1CON		104				//0x01A0	gpf1_0到gpf1_7	
#define M_GPF2CON		112				//0x01C0	gpf2_0到gpf2_7	
#define M_GPF3CON		120				//0x01E0	gpf3_0到gpf3_7	
		//0x1100_0000
#define M_GPL			0x11000000
#define M_GPL0_4		48				//0x00C0
#define M_GPL1_0		56				//0x00E0
		//gpio操作
#define GPF0_LCD		0x22222222
#define GPF1_LCD		0x22222222
#define GPF2_LCD		0x22222222
#define GPF3_LCD		0x2222<<0
#define GPD01_PWM		0x2<<4



/* 时钟相关的寄存器 */

#define M_CLK_SRC_TOP1              0x1003C214       //C214
		//0x1003_0000
#define M_CLKBASE		            0x1003C234
#define M_CLK_SRC_LCD0	            0				//C234
#define M_CLK_SRC_MASK_LCD          64              //C334
#define M_CLK_SRC_MASK_ISP          65              //C338
#define M_CLK_MUX_STAT_TOP1         120             //C414
#define M_CLK_DIV_LCD	            192				//C534
#define M_CLK_DIV_STAT_LCD          256             //C634
#define M_CLK_GATE_IP_LCD           448                 //C934
#define M_CLK_GATE_IP_ISP           449             //C938
#define M_CLK_GATE_BLOCK            463             //0xC970

#define M_CLK_RIGHTBUS_BASE         0x10038200
#define M_CLK_SRC_RIGHTBUS          0              //0x8200    
#define M_CLK_DIV_RIGHTBUS          192            //0x8500
#define M_CLK_GATE_IP_RIGHTBUS      384            //0x8800


		//0x1001_0000
#define	M_LCDBLKBASE	            0x10010000
#define M_LCDBLK_CFG	            132				//0x0210
#define M_LCDBLK_CFG2	            133				//0x0214
        //0x1004_0000
#define M_MPLL_CON0                 0x10040108    //0x010C


        //power
        //0x1002_1100
#define M_LCDPOWER_BASE                      0x10021100
#define	M_CMU_CLKSTOP_LCD0_SYS_PWR_REG       20           //0x1150
#define	M_CMU_RESET_LCD0_SYS_PWR_REG         28           //0x1170
#define	M_LCD0_SYS_PWR_REG                   164          // 0x1390
        //0x1002_3C80
#define M_LCDPOWER2_BASE                     0x10023C80
#define	M_LCD0_CONFIGURATION                 0         // 0x3C80
#define	M_LCD0_STATUS                        1        // 0x3C84
#define	M_LCD0_OPTION                        2        // 0x3C88



