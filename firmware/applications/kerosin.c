#include <sysinit.h>
#include "basic/basic.h"
#include "lcd/render.h"
#include "core/pmu/pmu.h"

#ifdef CFG_USBCDC
volatile unsigned int lastTick;
#include "core/usbcdc/usb.h"
#include "core/usbcdc/usbcore.h"
#include "core/usbcdc/usbhw.h"
#include "core/usbcdc/cdcuser.h"
#include "core/usbcdc/cdc_buf.h"
#endif

#ifdef CFG_INTERFACE_UART
	#include "core/uart/uart.h"
#endif

#define DMX_FORMAT_MAX	15

#define DMX_CHANNEL_MAX 512


#define DMX_RESET_COUNT	22
#define DMX_MARK_COUNT	26	/* 22 - 26 : 4ticks -> 16us */

uint8_t dmxChannelBuffer[DMX_CHANNEL_MAX];
uint8_t dmxFrameBuffer[DMX_FORMAT_MAX];

/* Use the 32bit timer for the DMX signal generation */
#include "core/timer32/timer32.h"

void handler(void);

void startTimer(void) {
    timer32Callback0 = handler;
    
    /* Enable the clock for CT32B0 */
    SCB_SYSAHBCLKCTRL |= (SCB_SYSAHBCLKCTRL_CT32B0);
    TMR_TMR32B0MR0  = 288; /*(72E6/250E3); /* frequency of 250kBit/s -> bit time of 4us */
    TMR_TMR32B0MCR = (TMR_TMR32B0MCR_MR0_INT_ENABLED | TMR_TMR32B0MCR_MR0_RESET_ENABLED);
    NVIC_EnableIRQ(TIMER_32_0_IRQn);
    TMR_TMR32B0TCR = TMR_TMR32B0TCR_COUNTERENABLE_ENABLED;
	
	/* FIXME debug stuff */
	dmxChannelBuffer[0] = 0x00; // red
	dmxChannelBuffer[1] = 0xFF; // green
	dmxChannelBuffer[2] = 0x00; // blue
	dmxChannelBuffer[3] = 0x00;
	dmxChannelBuffer[4] = 0xFF;
	dmxChannelBuffer[5] = 0x00;
	dmxChannelBuffer[6] = 0xFF;
	dmxChannelBuffer[7] = 0xFF;
	dmxChannelBuffer[8] = 0xFF;
	dmxChannelBuffer[9] = 0xFF;
	
}

void stopTimer(void) {
    NVIC_DisableIRQ(TIMER_32_0_IRQn);
    TMR_TMR32B0TCR = TMR_TMR32B0TCR_COUNTERENABLE_DISABLED;
}

/* build the next frame, that should be send 
   @param[in] data  the byte that should be send. */
void buildDMXframe(uint8_t data)
{
	dmxFrameBuffer[0] = 0; /* Startbit */
	dmxFrameBuffer[1] = (data & 1);
	dmxFrameBuffer[2] = (data & (1 << 1));
	dmxFrameBuffer[3] = (data & (1 << 2));
	dmxFrameBuffer[4] = (data & (1 << 3));
	dmxFrameBuffer[5] = (data & (1 << 4));
	dmxFrameBuffer[6] = (data & (1 << 5));
	dmxFrameBuffer[7] = (data & (1 << 6));
	dmxFrameBuffer[8] = (data & (1 << 7));
	dmxFrameBuffer[9] = 1; /* Stoppbit */
	dmxFrameBuffer[10] = 1; /* Stoppbit */
	
	dmxFrameBuffer[11] = 1; /* MARK zwischen Frames (Interdigit) */
	dmxFrameBuffer[12] = 1; /* MARK zwischen Frames (Interdigit) */
	dmxFrameBuffer[13] = 1; /* MARK zwischen Frames (Interdigit) */
	dmxFrameBuffer[14] = 1; /* MARK zwischen Frames (Interdigit) */
}

void handler(void)
{
	static int channelPtr=0;
	static int framePtr = 0;
	static int resetCounter = 0;
	
	/* make the reset flag */
	if (resetCounter < DMX_RESET_COUNT)
	{
		/* reset of 88us */
		gpioSetValue(RB_SPI_SS0, 0);
		resetCounter++;
		return;
	} else if (resetCounter >= DMX_RESET_COUNT && resetCounter < DMX_MARK_COUNT) {
		/* mark of 8us */
		gpioSetValue(RB_SPI_SS0, 1);
		resetCounter++;
		return;
	} else if (resetCounter == DMX_MARK_COUNT){
		/* build the startbyte */
		buildDMXframe(0);
		framePtr = 0; /* send the stop-bit */
		resetCounter++;
	} else if (resetCounter >= (DMX_MARK_COUNT + 1) 
			   && resetCounter < (DMX_MARK_COUNT + DMX_FORMAT_MAX)) {
		resetCounter++; /* Send the startbyte */
	} else if (resetCounter == (DMX_MARK_COUNT + DMX_FORMAT_MAX)) {		
		/* build first frame */		
		channelPtr = 0;
		framePtr = 0;
		buildDMXframe(dmxChannelBuffer[channelPtr++]);
		resetCounter = (DMX_MARK_COUNT + DMX_FORMAT_MAX) + 1; /* leave the beginning (reset and start byte) */
	} else {
		/* Handle normal state, when no start is used */		
		if (framePtr >= DMX_FORMAT_MAX)
		{
			/* reset frame pointer */
			framePtr = 0;
			
			if (channelPtr >= 2) {
				resetCounter = 0; /* activate RESET */
				return;
			}
			
			buildDMXframe(dmxChannelBuffer[channelPtr++]);
		}
	}


		

	
//	gpioSetValue(RB_SPI_SS0, dmxFrameBuffer[framePtr]);
	if (dmxFrameBuffer[framePtr] > 0) {
		gpioSetValue(RB_SPI_SS0, 1);
	} else {
		gpioSetValue(RB_SPI_SS0, 0);
	}

	
	framePtr++;
}

void main_kerosin(void) {

	uint8_t enterCnt = 0;
	uint8_t buffer[64];
	
//	cpuInit();                                // Configure the CPU
	systickInit(CFG_SYSTICK_DELAY_IN_MS);     // Start systick timer
	gpioInit();                               // Enable GPIO
//	pmuInit();                                // Configure power management
	adcInit();                                // Config adc pins to save power
	lcdInit();
		
    DoString(2,5,"USB");	
	
#ifdef CFG_INTERFACE_UART
	uartInit(CFG_UART_BAUDRATE);
	DoString(40,5,"UART");
	uartSend("Hallo", 5);
#endif

	// Initialise USB CDC
#ifdef CFG_USBCDC
    DoString(10, 15, "USBCDC");
    lastTick = systickGetTicks();   // Used to control output/printf timing
    CDC_Init();                     // Initialise VCOM
    USB_Init();                     // USB Initialization
    USB_Connect(TRUE);              // USB Connect
    // Wait until USB is configured or timeout occurs
    uint32_t usbTimeout = 0; 
    while ( usbTimeout < CFG_USBCDC_INITTIMEOUT / 10 )
    {
		if (USB_Configuration) break;
		systickDelay(10);             // Wait 10ms
		usbTimeout++;
    }
#endif
	  // Printf can now be used with UART or USBCDC
	
	puts("Hello World");

	
	DoString(10, 25, "Enter:");
	lcdDisplay();
	
	startTimer();
	DoString(1, 50, "Time enabled!");
	lcdDisplay();
	/* ---------------------------------------------- */

	int readData;
	int toggle=0;
    while (1) {
				
		switch (getInput()) {
			case BTN_ENTER:
				enterCnt+=10;
				/* This is debug code */
				puts("ENTER\t");
				buffer[0] = '0' + enterCnt;
				buffer[1] = 0;
				puts(buffer);
				puts("\r\n");
				dmxChannelBuffer[1] = enterCnt; 
				DoInt(50, 25, (int) (enterCnt));
				lcdDisplay();				
				break;
			case BTN_LEFT:
				readData = CDC_GetInputBuffer(buffer, sizeof(buffer));				
				DoString(5, 40, buffer);
				DoString(1, 50, "RX-Cnt:");
				DoInt(60, 50, readData);
				lcdDisplay();
				break;
			default:
				break;
		}		
	}
	
	stopTimer();
}

#ifdef CFG_USBCDC
/**************************************************************************/
/*! 
 @brief Sends a string to a pre-determined end point (UART, etc.).
 
 @param[in]  str
 Text to send
 
 @note This function is only called when using the GCC-compiler
 in Codelite or running the Makefile manually.  This function
 will not be called when using the C library in Crossworks for
 ARM.
 */
/**************************************************************************/
int puts(const char * str)
{
	// There must be at least 1ms between USB frames (of up to 64 bytes)
	// This buffers all data and writes it out from the buffer one frame
	// and one millisecond at a time
#ifdef CFG_PRINTF_USBCDC
    if (USB_Configuration) 
    {
		while(*str)
			cdcBufferWrite(*str++);
		// Check if we can flush the buffer now or if we need to wait
		unsigned int currentTick = systickGetTicks();
		if (currentTick != lastTick)
		{
			uint8_t frame[64];
			uint32_t bytesRead = 0;
			while (cdcBufferDataPending())
			{
				// Read up to 64 bytes as long as possible
				bytesRead = cdcBufferReadLen(frame, 64);
				USB_WriteEP (CDC_DEP_IN, frame, bytesRead);
				systickDelay(1);
			}
			lastTick = currentTick;
		}
    }
#else
    // Handle output character by character in __putchar
    while(*str) __putchar(*str++);
#endif
	
	return 0;
}
#endif