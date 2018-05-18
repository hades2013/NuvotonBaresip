/* linux/driver/input/w55fa92_ts.c
 *
 * Copyright (c) 2013 Nuvoton technology corporation
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Changelog:
 *
 *   2013/07/11   SWChou add this file for nuvoton touch screen driver.
 */
 


#include <linux/init.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/w55fa92_reg.h>
#undef BIT
#include <linux/input.h>
#include <linux/clk.h>
#include "w55fa92_ts_adc.h"

#ifdef CONFIG_BATTERY_DETECTION
extern struct timer_list battery_timer;
#endif

//#define DBG
#ifdef DBG
#define ENTER()		printk("ENTER : %s  : %s\n",__FILE__, __FUNCTION__)
#define LEAVE()		printk("LEAVE : %s  : %s\n",__FILE__, __FUNCTION__)
#define ERRLEAVE()	printk("ERRLEAVE : %s  : %s\n",__FILE__, __FUNCTION__)
#define DBG_PRINTF	printk
#else
#define ENTER(...)
#define LEAVE(...)	
#define ERRLEAVE(...)	
#define DBG_PRINTF(...)	
#endif

static PFN_ADC_CALLBACK (g_psVoltageDetectionCallBack)[1] = {0};
static void VoltageDetect_callback(UINT32 u32code)
{
	ENTER();
	if( g_psVoltageDetectionCallBack[0]!=0)
		g_psVoltageDetectionCallBack[0](u32code);	
	LEAVE();
}

void Install_VoltageDetectionCallback(PFN_ADC_CALLBACK pfnCallback)
{	
	ENTER();
	g_psVoltageDetectionCallBack[0] = pfnCallback; 
	LEAVE();
}


void timer_check_battery(unsigned long dummy)
{
	INT32 i32ret;
	UINT32 u32Channel;
	
	ENTER();
#ifdef CONFIG_BATTERY_CHANNEL_1
	u32Channel = 1;
#endif
#ifdef CONFIG_BATTERY_CHANNEL_2	
	u32Channel = 2;
#endif
#ifdef CONFIG_BATTERY_CHANNEL_3	
	u32Channel = 3;
#endif
	i32ret = DrvADC_VoltageDetection(u32Channel);//Channel 
	if(i32ret==E_ADC_BUSY){//do again after 10ms
		DBG_PRINTF("BZ\n");
		//mod_timer(&battery_timer, jiffies + INIT_INTERVAL_TIME); 
		mod_timer(&battery_timer, jiffies + BATTERY_INTERVAL_TIME); 
		LEAVE();
		return; 
	}
	mod_timer(&battery_timer, jiffies + BATTERY_INTERVAL_TIME); 
	LEAVE();
}
void init_batterydetection(void)
{
	PFN_ADC_CALLBACK pfnOldCallback;

	ENTER();
	DrvADC_InstallCallback(eADC_AIN,
						VoltageDetect_callback,
						&pfnOldCallback);
	LEAVE();
}
