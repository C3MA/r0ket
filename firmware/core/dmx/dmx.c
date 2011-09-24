#include "basic/basic.h"
#include "core/dmx/dmx.h"

/* Lenght of dmxFrameBuffer, 
 * that descibes one byte including start-bit, stop-bits 
 * and mark between two bytes/channel (intertigit) */
#define DMX_FORMAT_MAX	20

#define DMX_BREAK	0
#define DMX_MARK	1

/* define some constants for the reset */
#define COUNTER_RESET_END	400
#define COUNTER_MARK_END	(COUNTER_RESET_END + 100)	/* 100ticks -> 400us */
#define COUNTER_PREAMPLE_END	(COUNTER_MARK_END + DMX_FORMAT_MAX) + 1

#define COUNTER_POSTMARK	(COUNTER_PREAMPLE_END + 10) /* needed to mark the end of a package */
#define COUNTER_POSTMARK_END	(COUNTER_POSTMARK + 100) /* needed to mark the end of a package */


static uint8_t dmxChannelBuffer[DMX_CHANNEL_MAX]; /*FIXME make it private */
static uint8_t dmxFrameBuffer[DMX_FORMAT_MAX];

/* Use the 32bit timer for the DMX signal generation */
#include "core/timer32/timer32.h"

void handler(void);

extern void dmx_getDMXbuffer(uint8_t** ppBuffer)
{
	uint8_t *pbuffer = dmxChannelBuffer;
	(*ppBuffer) = pbuffer;
}

extern void dmx_setChannel(int channel, uint8_t value)
{
	if ((channel >= DMX_FORMAT_MAX) || (channel < 0))
		return;
	
	/* correct channel could be set */
	dmxChannelBuffer[channel] = value;
}

extern void dmx_setLightBox(int box, uint8_t red, uint8_t green, uint8_t blue)
{
	int maxBox = DMX_FORMAT_MAX / 4;
	if (box < 0 || box >= maxBox)
		return;
	
	dmx_setChannel(box * 4, red);
	dmx_setChannel((box * 4) + 1, green);
	dmx_setChannel((box * 4) + 2, blue);
}

extern void dmx_start(void) {
    timer32Callback0 = handler;
    
    /* Enable the clock for CT32B0 */
    SCB_SYSAHBCLKCTRL |= (SCB_SYSAHBCLKCTRL_CT32B0);
    TMR_TMR32B0MR0  = 288; /*(72E6/250E3); frequency of 250kBit/s -> bit time of 4us */
    TMR_TMR32B0MCR = (TMR_TMR32B0MCR_MR0_INT_ENABLED | TMR_TMR32B0MCR_MR0_RESET_ENABLED);
    NVIC_EnableIRQ(TIMER_32_0_IRQn);
    TMR_TMR32B0TCR = TMR_TMR32B0TCR_COUNTERENABLE_ENABLED;
	
	/* FIXME debug stuff */
	dmxChannelBuffer[0] = 0x00; // red
	dmxChannelBuffer[1] = 0xFF; // green
#if 0
	dmxChannelBuffer[2] = 0x00; // blue
	dmxChannelBuffer[3] = 0x00;
	dmxChannelBuffer[4] = 0xFF;
	dmxChannelBuffer[5] = 0x00;
	dmxChannelBuffer[6] = 0xFF;
	dmxChannelBuffer[7] = 0xFF;
	dmxChannelBuffer[8] = 0xFF;
	dmxChannelBuffer[9] = 0xFF;
#endif
}

extern void dmx_stop(void) {
    NVIC_DisableIRQ(TIMER_32_0_IRQn);
    TMR_TMR32B0TCR = TMR_TMR32B0TCR_COUNTERENABLE_DISABLED;
}

/* build the next frame, that should be send 
 @param[in] data  the byte that should be send. */
void buildDMXframe(uint8_t data)
{
	dmxFrameBuffer[0] = 0; /* Startbit */
	dmxFrameBuffer[1] = !(data & 1);
	dmxFrameBuffer[2] = !(data & (1 << 1));
	dmxFrameBuffer[3] = !(data & (1 << 2));
	dmxFrameBuffer[4] = !(data & (1 << 3));
	dmxFrameBuffer[5] = !(data & (1 << 4));
	dmxFrameBuffer[6] = !(data & (1 << 5));
	dmxFrameBuffer[7] = !(data & (1 << 6));
	dmxFrameBuffer[8] = !(data & (1 << 7));
	dmxFrameBuffer[9] = 1; /* Stoppbit */
	dmxFrameBuffer[10] = 1; /* Stoppbit */
	
	dmxFrameBuffer[11] = 1; /* MARK zwischen Frames (Interdigit) */
	dmxFrameBuffer[12] = 1; /* MARK zwischen Frames (Interdigit) */
	dmxFrameBuffer[13] = 1; /* MARK zwischen Frames (Interdigit) */
	dmxFrameBuffer[14] = 1; /* MARK zwischen Frames (Interdigit) */
	dmxFrameBuffer[15] = 1; /* MARK zwischen Frames (Interdigit) */
	dmxFrameBuffer[16] = 1; /* MARK zwischen Frames (Interdigit) */
	dmxFrameBuffer[17] = 1; /* MARK zwischen Frames (Interdigit) */
	dmxFrameBuffer[18] = 1; /* MARK zwischen Frames (Interdigit) */
	dmxFrameBuffer[19] = 1; /* MARK zwischen Frames (Interdigit) */
}

void handler(void)
{
	static int channelPtr=0;
	static int framePtr = 0;
	static int resetCounter = 0;
	
	/* make the reset flag */
	if (resetCounter < COUNTER_RESET_END)
	{
		/* reset of minimum 88us */
		gpioSetValue(RB_SPI_SS0, DMX_MARK);
		resetCounter++;
		return;
	} else if (resetCounter >= COUNTER_RESET_END && resetCounter < COUNTER_MARK_END) {
		/* mark of 8us */
		gpioSetValue(RB_SPI_SS0, DMX_BREAK);
		resetCounter++;
		return;
	} else if (resetCounter == COUNTER_MARK_END){
		/* build the startbyte */
		buildDMXframe(0);
		framePtr = 0; /* send the stop-bit */
		resetCounter++;
	} else if (resetCounter >= (COUNTER_MARK_END + 1) 
			   && resetCounter < (COUNTER_MARK_END + DMX_FORMAT_MAX)) {
		resetCounter++; /* Send the startbyte */
	} else if (resetCounter == (COUNTER_MARK_END + DMX_FORMAT_MAX)) {		
		/* build first frame */		
		channelPtr = 0;
		framePtr = 0;
		buildDMXframe(dmxChannelBuffer[channelPtr++]);
		resetCounter = COUNTER_PREAMPLE_END; /* leave the beginning (reset and start byte) */
		
		/*-------------- this build a mark at the end between two packages -------*/		
	} else if (resetCounter >= COUNTER_POSTMARK_END) { /* first check if the end has been reached */
		/* Rest the counter and begin from the beginning */
		resetCounter = 0;
		return;
	} else if (resetCounter >= COUNTER_POSTMARK) {
		gpioSetValue(RB_SPI_SS0, DMX_BREAK);
		resetCounter++;
		return;
		
	} else {
		/* Handle normal state, when no start is used */		
		if (framePtr >= 10)
		{
			/* reset frame pointer */
			framePtr = 0;
			
			if (channelPtr >= 20) {
				resetCounter = COUNTER_POSTMARK; /* jump to the end and build the end-mark */
				return;
			}
			
			buildDMXframe(dmxChannelBuffer[channelPtr++]);
		}
	}
	
	
	
	//	gpioSetValue(RB_SPI_SS0, dmxFrameBuffer[framePtr]);
	if (dmxFrameBuffer[framePtr] > 0) {
		gpioSetValue(RB_SPI_SS0, DMX_BREAK);
	} else {
		gpioSetValue(RB_SPI_SS0, DMX_MARK);
	}
	
	
	framePtr++;
}