/* sensor_ctl.c
 *
 * Copyright (c) 2008 Nuvoton technology corporation
 * All rights reserved.
 * <clyu2@nuvoton.com>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
 
//#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/smp_lock.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#if 0
#include <linux/videodev.h>
#endif
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <linux/jiffies.h>
#if 0
#include <asm/arch/videoin.h>
#include <asm/arch/DrvVideoin.h>
#include <asm/arch/w55fa93_reg.h>
#include <asm/arch/fb.h>
#include <asm/arch/w55fa93_fb.h>
#include <asm/arch/w55fa93_gpio.h>
#endif
#include <mach/w55fa92_reg.h>
#include <mach/fb.h>
#include <mach/w55fa92_fb.h>
#include <mach/videoin.h>
//#include <mach/DrvVideoin.h>
#include "DrvVideoin.h"
#include <mach/videodev_ex.h>
#include <mach/w55fa92_gpio.h>

#include <asm/io.h>
#include <linux/i2c.h>

#include "videoinpriv.h"
#include "DrvI2C.h" 
#ifdef DBG
#define ENTER()		printk("ENTER : %s  : %s\n",__FILE__, __FUNCTION__)
#define LEAVE()		printk("LEAVE : %s  : %s\n",__FILE__, __FUNCTION__)
#define ERRLEAVE()	printk("ERRLEAVE : %s  : %s : %d\n",__FILE__, __FUNCTION__, __LINE__)
#define SDBG		printk	
#else
#define ENTER(...)
#define LEAVE(...)	
#define ERRLEAVE(...)	
#define SDBG(...)
#endif


#ifdef CONFIG_DEV_BOARD_DEV1
void Snr1_Reset(BOOL bIsReset)
{/* GPB04 reset:	H->L->H */	
	printk("Sensor reset\n");		
  #if CONFIG_SUPPORT_SENSOR_RESET_DEV1
	outp32(REG_GPBFUN0, inp32(REG_GPBFUN0) & (~ MF_GPB4));
	outp32(REG_GPIOB_OMD, inp32(REG_GPIOB_OMD) | BIT4);
	outp32(REG_GPIOB_DOUT, inp32(REG_GPIOB_DOUT) | BIT4);
	udelay(100);
	outp32(REG_GPIOB_DOUT, inp32(REG_GPIOB_DOUT) & ~BIT4);
	udelay(100);
	outp32(REG_GPIOB_DOUT, inp32(REG_GPIOB_DOUT) | BIT4);
  #endif
}
void Snr1_PowerDown(BOOL bIsEnable)
{/* GPE8 power down, HIGH for power down */
	printk("Sensor power down\n");		
  #ifdef CONFIG_SUPPORT_SENSOR_POWER_DOWN_DEV1
	outp32(REG_GPEFUN1, inp32(REG_GPEFUN1) & (~ MF_GPE8));
	outp32(REG_GPIOE_OMD, inp32(REG_GPIOE_OMD) | BIT8);
	outp32(REG_GPIOE_DOUT, inp32(REG_GPIOE_DOUT) & ~BIT8);	
	if(bIsEnable){
		outp32(REG_GPIOE_DOUT, inp32(REG_GPIOE_DOUT) | BIT8);
		printk("Sensor power down\n");
	}
	else{			
		printk("Sensor not to power down\n");
		outp32(REG_GPIOE_DOUT, inp32(REG_GPIOE_DOUT) & ~BIT8);	
	}	
  #endif
}
#endif
#ifdef CONFIG_DEMO_BOARD_DEV1
void Snr1_Reset(BOOL bIsReset)
{/*  GPE8 reset  */				
	printk("Sensor reset\n");		
  #if CONFIG_SUPPORT_SENSOR_RESET_DEV1
	outp32(REG_GPEFUN1, inp32(REG_GPEFUN1) & (~ MF_GPE8));
	outp32(REG_GPIOE_OMD, inp32(REG_GPIOE_OMD) | BIT8);
	outp32(REG_GPIOE_DOUT, inp32(REG_GPIOE_DOUT) | BIT8);
	udelay(100);
	outp32(REG_GPIOE_DOUT, inp32(REG_GPIOE_DOUT) & ~BIT8);
	udelay(100);
	outp32(REG_GPIOE_DOUT, inp32(REG_GPIOE_DOUT) | BIT8);
  #endif
}
void Snr1_PowerDown(BOOL bIsEnable)
{
	printk("Sensor power down\n");	
  #ifdef CONFIG_SUPPORT_SENSOR_POWER_DOWN_DEV1

  #endif
}
#endif
#ifdef CONFIG_VIDEODOOR_BOARD_DEV1
void Snr1_Reset(BOOL bIsReset)
{/* GPB04 reset:	H->L->H */	
	printk("Sensor reset\n");		
  #if CONFIG_SUPPORT_SENSOR_RESET_DEV1
	outp32(REG_GPBFUN0, inp32(REG_GPBFUN0) & (~ MF_GPB4));
	outp32(REG_GPIOB_OMD, inp32(REG_GPIOB_OMD) | BIT4);
	outp32(REG_GPIOB_DOUT, inp32(REG_GPIOB_DOUT) | BIT4);
	udelay(100);
	outp32(REG_GPIOB_DOUT, inp32(REG_GPIOB_DOUT) & ~BIT4);
	udelay(100);
	outp32(REG_GPIOB_DOUT, inp32(REG_GPIOB_DOUT) | BIT4);
  #endif
}
void Snr1_PowerDown(BOOL bIsEnable)
{/* GPD15 power down, HIGH for power down */
	printk("Sensor power down\n");
  #ifdef CONFIG_SUPPORT_SENSOR_POWER_DOWN_DEV1
	outp32(REG_GPDFUN1, inp32(REG_GPDFUN1) & (~ MF_GPD15));
	outp32(REG_GPIOD_OMD, inp32(REG_GPIOD_OMD) | BIT15);
	outp32(REG_GPIOD_DOUT, inp32(REG_GPIOD_DOUT) & ~BIT15);	
	if(bIsEnable){
		outp32(REG_GPIOD_DOUT, inp32(REG_GPIOD_DOUT) | BIT15);
		printk("Sensor power down\n");
	}
	else{			
		printk("Sensor not to power down\n");
		outp32(REG_GPIOD_DOUT, inp32(REG_GPIOD_DOUT) & ~BIT15);	
	}	
  #endif
}
#endif
