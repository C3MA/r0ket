
#ifndef _H_DMX
#define	_H_DMX

#include <sysinit.h>

#define DMX_CHANNEL_MAX 512

/*
 * This function starts the dmx_timer and sets up all needed register.
 */
extern void dmx_start(void);

/*
 * This functions stops all the cool dmx stuff :-(
 */
extern void dmx_stop(void);

/* Simple method for setting one channel.
 * Set a single value for a channel
 */
extern void dmx_setChannel(int channel, uint8_t value);

/*
 * This get a pointer of the internal used buffer, to modify the whole universe.
 * (You should know, what you do... sometimes ;-) )
 */
extern void dmx_getDMXbuffer(uint8_t** ppBuffer);

#endif