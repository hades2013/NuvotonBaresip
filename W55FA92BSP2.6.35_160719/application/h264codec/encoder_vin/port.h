#ifndef _PORT_H_
#define _PORT_H_


#include <stdint.h>

#define TRUE    1
#define FALSE   0

#define UINT32 unsigned int

typedef unsigned char	Uint8;
typedef unsigned int	Uint32;
typedef unsigned short	Uint16;

#define CACHE_LINE  32
#define ptr_t uint32_t
#define		CACHE_BIT31 0x80000000

//#define	NULL	0
//#define mdelay	sysDelay

#define DIV_16(n)	((n+0x0000F)/0x00010)*0x00010

#ifdef LINUX_ENV
#define M_DEBUG printk
#define F_DEBUG     printk
#define Console_Printf printk
#define sysprintf printk

#define outp32(addr,value)	outl((unsigned long)value, (unsigned int)addr)
#define inp32(addr)		inl((unsigned int)addr)



#else
#define	M_DEBUG	sysprintf
#define printk sysprintf
#endif

#endif

