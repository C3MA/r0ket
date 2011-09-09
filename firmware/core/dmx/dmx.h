
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

/*
 * This get a pointer of the internal used buffer, to modify the package
 */
extern void dmx_getDMXbuffer(uint8_t** ppBuffer);

#endif