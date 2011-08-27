/**
 * Orignal build for Arduino [1]... this is a port to ARM
 * --- orginal header ---
 * DmxSimple - A simple interface to DMX.
 *
 * Copyright (c) 2008-2009 Peter Knight, Tinker.it! All rights reserved.
 * ---
 *
 * [1] http://code.google.com/p/tinkerit/wiki/DmxSimple 
 */

#ifndef DmxSimple_h
#define DmxSimple_h

#define DMX_SIZE 512

extern void dmx_maxChannel(int);
extern void dmx_write(int, uint8_t);
extern void dmx_usePin(uint8_t);

#endif

