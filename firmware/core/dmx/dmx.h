
#ifndef _H_DMX
#define	_H_DMX

#include <sysinit.h>

//#define DMX_CHANNEL_MAX 512
#define DMX_CHANNEL_MAX 5

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
 * @param[in] channel we are from IT ... counting from zero
 * @param[in] value we are from IT ... counting from 0 to 255
 */
extern void dmx_setChannel(int channel, uint8_t value);


/*
 * set a single lightbox
 * @param[in] box	the box we want to select (from zero to ... 64 afaik)
 * @param[in] red	we are from IT ... counting from 0 to 255
 * @param[in] green	we are from IT ... counting from 0 to 255
 * @param[in] blue	we are from IT ... counting from 0 to 255
 */
extern void dmx_setLightBox(int box, uint8_t red, uint8_t green, uint8_t blue);

/*
 * This get a pointer of the internal used buffer, to modify the whole universe.
 * (You should know, what you do... sometimes ;-) )
 */
extern void dmx_getDMXbuffer(uint8_t** ppBuffer);

#endif