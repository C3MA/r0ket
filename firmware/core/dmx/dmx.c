#include "basic/basic.h"
#include "core/dmx/dmx.h"

/* Lenght of dmxFrameBuffer, 
 * that descibes one byte including start-bit, stop-bits 
 * and mark between two bytes/channel (intertigit) */
#define DMX_FORMAT_MAX	20
#define DMX_FORMAT_WITHOUT_INTERFRAMESPACE	11

#define DMX_BREAK	0
#define DMX_MARK	1

/****** define some constants for the reset; IMPORTANT: on tick = 4us *******/
#define COUNTER_RESET_END	22				/* Last tick of RESET */ 
#define COUNTER_MARK_END	(COUNTER_RESET_END + 2)	/* Last tick of the MARK */
#define COUNTER_PREAMPLE_END	(COUNTER_MARK_END + DMX_FORMAT_WITHOUT_INTERFRAMESPACE) + 1

#define COUNTER_POSTMARK	(COUNTER_PREAMPLE_END + 10) /* first tick of the end of a frame */
#define COUNTER_POSTMARK_END	(COUNTER_POSTMARK + 50) /* last tick of the end of a frame */

#define EXTR_LEVEL(a)			(((a) > 0) ? DMX_MARK : DMX_BREAK)

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
	dmxChannelBuffer[0] = 0xFF; // red
	dmxChannelBuffer[1] = 0x05; // green
	dmxChannelBuffer[2] = 0xFF; // blue
	dmxChannelBuffer[3] = 0x05;
	dmxChannelBuffer[4] = 0xFF;
	dmxChannelBuffer[5] = 0x05;
	dmxChannelBuffer[6] = 0xFF;
	dmxChannelBuffer[7] = 0x05;
	dmxChannelBuffer[8] = 0xFF;
	dmxChannelBuffer[9] = 0x05;
}

extern void dmx_stop(void) {
    NVIC_DisableIRQ(TIMER_32_0_IRQn);
    TMR_TMR32B0TCR = TMR_TMR32B0TCR_COUNTERENABLE_DISABLED;
}

/* build the next frame, that should be send 
 @param[in] data  the byte that should be send. */
void buildDMXframe(uint8_t data)
{
	dmxFrameBuffer[0] = DMX_BREAK; /* Startbit */
	dmxFrameBuffer[1] = EXTR_LEVEL((data & 1));
	dmxFrameBuffer[2] = EXTR_LEVEL((data & (1 << 1)));
	dmxFrameBuffer[3] = EXTR_LEVEL((data & (1 << 2)));
	dmxFrameBuffer[4] = EXTR_LEVEL((data & (1 << 3)));
	dmxFrameBuffer[5] = EXTR_LEVEL((data & (1 << 4)));
	dmxFrameBuffer[6] = EXTR_LEVEL((data & (1 << 5)));
	dmxFrameBuffer[7] = EXTR_LEVEL((data & (1 << 6)));
	dmxFrameBuffer[8] = EXTR_LEVEL((data & (1 << 7)));
	dmxFrameBuffer[9] =  DMX_MARK;	/* Stoppbit */
	dmxFrameBuffer[10] = DMX_MARK;	/* Stoppbit */
	
	dmxFrameBuffer[11] = DMX_MARK; /* MARK between two packages (Interdigit) */
	dmxFrameBuffer[12] = DMX_MARK; /* MARK between two packages (Interdigit) */
	dmxFrameBuffer[13] = DMX_MARK; /* MARK between two packages (Interdigit) */
	dmxFrameBuffer[14] = DMX_MARK; /* MARK between two packages (Interdigit) */
	dmxFrameBuffer[15] = DMX_MARK; /* MARK between two packages (Interdigit) */
	dmxFrameBuffer[16] = DMX_MARK; /* MARK between two packages (Interdigit) */
	dmxFrameBuffer[17] = DMX_MARK; /* MARK between two packages (Interdigit) */
	dmxFrameBuffer[18] = DMX_MARK; /* MARK between two packages (Interdigit) */
	dmxFrameBuffer[19] = DMX_MARK; /* MARK between two packages (Interdigit) */
}

void handler(void)
{
	static int channelPtr=0;
	static int framePtr = 0;
	static int resetCounter = 0;
	static int nextPinState = DMX_BREAK;
	
	/* set the level at the pin */
	gpioSetValue(RB_SPI_SS0, nextPinState);	
	
	/* -------- calculated the next byte ---------- */ 
	
	/* make the reset flag */
	if (resetCounter < COUNTER_RESET_END)
	{
		/* reset of minimum 88us */
		nextPinState = DMX_BREAK;
		resetCounter++;
		return;
	} else if (resetCounter >= COUNTER_RESET_END && resetCounter < COUNTER_MARK_END) {
		nextPinState = DMX_MARK;
		resetCounter++;
		return;
	} else if (resetCounter == COUNTER_MARK_END){
		/* build the startbyte */
		buildDMXframe(0x00);
		framePtr = 0; /* send the stop-bit */
		resetCounter++;
	} else if (resetCounter >= (COUNTER_MARK_END + 1) 
			   && resetCounter < (COUNTER_MARK_END + DMX_FORMAT_WITHOUT_INTERFRAMESPACE)) {
		resetCounter++; /* Send the startbyte */
	} else if (resetCounter == (COUNTER_MARK_END + DMX_FORMAT_WITHOUT_INTERFRAMESPACE)) {		
		/* build first frame */		
		channelPtr = 0;
		framePtr = 0;
		buildDMXframe(dmxChannelBuffer[channelPtr++]);
		resetCounter = COUNTER_PREAMPLE_END; /* leave the beginning (reset and start byte) */
		
		/*-------------- this build a mark at the end between two packages -------*/		
	} else if (resetCounter >= COUNTER_POSTMARK_END) { /* first check if the end has been reached */
		/* Rest the counter and start from the beginning */
		resetCounter = 0;
		return;
	} else if (resetCounter >= COUNTER_POSTMARK) {
		/* build the mark between to frames until COUNTER_POSTMARK_END */
		nextPinState = DMX_MARK;
		resetCounter++;
		return;
	} else {
		/* Handle normal state, when no start or end is used, the channels must be transmitted */	
		
		/* FIXME: do not send an interdigit between two channels (others does this every 8 channel) */
		if (framePtr >= DMX_FORMAT_WITHOUT_INTERFRAMESPACE)
		{
			/* reset frame pointer */
			framePtr = 0;
			
			if (channelPtr >= DMX_CHANNEL_MAX) {
				resetCounter = COUNTER_POSTMARK; /* jump to the end and build the end-mark */
				nextPinState = DMX_MARK; /* build the mark between to frames */
				return;
			}
			buildDMXframe(dmxChannelBuffer[channelPtr++]);
		}
	}
	/* store the calculated value und send it in the next cycle */
	nextPinState = dmxFrameBuffer[framePtr];
	framePtr++;
}
