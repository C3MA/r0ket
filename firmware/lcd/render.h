#ifndef __RENDER_H_
#define __RENDER_H_

#include "display.h"

/*
#badge    <s> 96x68
#badge    <s> das farbige hat 98x70 
#badge    <s> das laengliche 128x32 
*/

#define RESX 96
#define RESY 68

// ARM supports byte flip natively. Yay!
#define flip(byte) \
	__asm("rbit %[value], %[value];" \
		  "rev  %[value], %[value];" \
			: [value] "+r" (byte) : )

/* Alternatively you could use normal byte flipping:
#define flip(c) do {\
	c = ((c>>1)&0x55)|((c<<1)&0xAA); \
	c = ((c>>2)&0x33)|((c<<2)&0xCC); \
	c = (c>>4) | (c<<4); \
	}while(0)
*/

int DoChar(int sx, int sy, int c);
int DoString(int sx, int sy, char *s);
int DoInt(int sx, int sy, int num);
int DoIntX(int sx, int sy, unsigned int num);

#endif
