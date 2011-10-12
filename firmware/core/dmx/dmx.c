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

typedef enum dmx_states_e { Idle, Reset, Stop, Start, Send } dmx_states_t;
typedef enum dmx_mode_e	{ Continous, Single, Off} dmx_mode_t;

static dmx_frame_t dmx_next_frame;
static uint8_t dmx_frame_ready = 00;
static dmx_mode_t dmx_mode = Off;

/* Callback function, that is called, when stuff could be done while STOP aka. MARK */
static void (*handleCharsWhileMark) (uint8_t* buffer) = NULL;

/* Use the 32bit timer for the DMX signal generation */
#include "core/timer32/timer32.h"

void handler(void);

extern void dmx_getDMXbuffer(uint8_t** ppBuffer)
{
//	uint8_t *pbuffer = dmx_next_frame.channels;
	(*ppBuffer) = dmx_next_frame.channels;
}

extern void dmx_setChannel(int channel, uint8_t value)
{
	if ((channel >= DMX_FORMAT_MAX) || (channel < 0))
		return;
	
	/* correct channel could be set */
	dmx_next_frame.channels[channel] = value;
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

extern void dmx_init(void)
{
    timer32Callback0 = handler;
    
    /* Enable the clock for CT32B0 */
    SCB_SYSAHBCLKCTRL |= (SCB_SYSAHBCLKCTRL_CT32B0);
    TMR_TMR32B0MR0  = 288; /*(72E6/250E3); frequency of 250kBit/s -> bit time of 4us */
    TMR_TMR32B0MCR = (TMR_TMR32B0MCR_MR0_INT_ENABLED | TMR_TMR32B0MCR_MR0_RESET_ENABLED);
	
	/* Update the priority */
	/* first WAKEUP0_IRQn is 0 ... last is EINT0_IRQn = 56 -> Hack to update all priority */
	for (int i=0; i <= 56; i++) {
		NVIC_SetPriority(i, 0x1F);
	}
	NVIC_SetPriority(TIMER_32_0_IRQn, 0x00);
	
    NVIC_EnableIRQ(TIMER_32_0_IRQn);
    TMR_TMR32B0TCR = TMR_TMR32B0TCR_COUNTERENABLE_ENABLED;
}

extern void dmx_deinit(void)
{
    NVIC_DisableIRQ(TIMER_32_0_IRQn);
    TMR_TMR32B0TCR = TMR_TMR32B0TCR_COUNTERENABLE_DISABLED;
}

extern void dmx_start(void) {
    dmx_mode = Continous;
	dmx_next_frame.No_channels = 512;
}

extern void dmx_stop(void) {
    dmx_mode = Off;
}

void handler(void)
{
	static dmx_frame_t dmx_frame;
	static dmx_states_t state = Idle;
	static uint8_t counter;
	static uint16_t channel_counter = 0;
	static uint8_t current_channel;
	
	static uint8_t out = 0; 
	
	gpioSetValue(RB_SPI_SS0, out);
	
	switch(state)
	{
		case(Idle):
			out = 1;
			switch(dmx_mode)
			{
				case(Single):
					if(dmx_frame_ready)
					{
						/* swap_frame */
						dmx_frame = dmx_next_frame;
						
						state = Reset;
						counter = 0;
					}
					break;
				case(Continous):
					/* swap_frame */
					dmx_frame = dmx_next_frame;

					state = Reset;
					counter = 0;
					break;
				default:
				case(Off):
					break;
			}
			break;
		case(Reset):
			out = 0;
			if(++counter >= COUNTER_RESET_END)
			{
				state = Stop;
				channel_counter = -1;
				counter = 0;
			}
			break;
		case(Stop):
			out = 1;
			if(counter++ >= 1)
			{
				state = Start;
				channel_counter++;
				if(channel_counter == 0)
				{
					current_channel = 0x00; 
					state = Start;
				}
				else if (channel_counter > dmx_frame.No_channels)
				{
					state = Idle;
				}
				else
				{
					current_channel = dmx_frame.channels[channel_counter-1];
					state = Start;
					if (handleCharsWhileMark != NULL)
						handleCharsWhileMark(dmx_next_frame.channels);
				}
			}
			break;
		case(Start):
			out = 0;
			state = Send;
			counter = 0;
			break;
		case(Send):
			out = current_channel & 0x1;
			current_channel >>= 1;
			if(++counter >= 8)
			{
				state = Stop;
				counter = 0;
			}
			break;
	}
}

extern void dmx_setHandler(void (*handleChars) (uint8_t* buffer))
{
	handleCharsWhileMark = handleChars;
}
