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

uint8_t dmxChannelBuffer[DMX_CHANNEL_MAX];
uint8_t dmxFormatBuffer[DMX_FORMAT_MAX];

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
	dmxChannelBuffer[0] = 0x41;
	dmxChannelBuffer[1] = 0xAA;
	dmxChannelBuffer[2] = 0x0;
	dmxChannelBuffer[3] = 0xFF;
	dmxChannelBuffer[4] = 0;
	
	dmxFormatBuffer[0] = 0; /* Startbit */
    dmxFormatBuffer[1] = (dmxChannelBuffer[0] & 1);
	dmxFormatBuffer[2] = (dmxChannelBuffer[0] & (1 << 1));
	dmxFormatBuffer[3] = (dmxChannelBuffer[0] & (1 << 2));
	dmxFormatBuffer[4] = (dmxChannelBuffer[0] & (1 << 3));
	dmxFormatBuffer[5] = (dmxChannelBuffer[0] & (1 << 4));
	dmxFormatBuffer[6] = (dmxChannelBuffer[0] & (1 << 5));
	dmxFormatBuffer[7] = (dmxChannelBuffer[0] & (1 << 6));
	dmxFormatBuffer[8] = (dmxChannelBuffer[0] & (1 << 7));
	dmxFormatBuffer[9] = 1; /* Stoppbit */
	dmxFormatBuffer[10] = 1; /* Stoppbit */
	
	dmxFormatBuffer[11] = 1; /* MARK zwischen Frames (Interdigit) */
	dmxFormatBuffer[12] = 1; /* MARK zwischen Frames (Interdigit) */
	dmxFormatBuffer[13] = 1; /* MARK zwischen Frames (Interdigit) */
	dmxFormatBuffer[14] = 1; /* MARK zwischen Frames (Interdigit) */
}

void stopTimer(void) {
    NVIC_DisableIRQ(TIMER_32_0_IRQn);
    TMR_TMR32B0TCR = TMR_TMR32B0TCR_COUNTERENABLE_DISABLED;
}


void handler(void)
{
	static int channelPtr=0;
	static int formatPtr = 0;
	
	if (formatPtr >= 11) //DMX_FORMAT_MAX
	{
		formatPtr = 0;
	}
	
//	gpioSetValue(RB_SPI_SS0, dmxFormatBuffer[formatPtr]);
	if (dmxFormatBuffer[formatPtr] > 0) {
		gpioSetValue(RB_SPI_SS0, 1);
	} else {
		gpioSetValue(RB_SPI_SS0, 0);
	}

	
	formatPtr++;
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
				enterCnt++;
				/* This is debug code */
				puts("ENTER\t");
				buffer[0] = '0' + enterCnt;
				buffer[1] = 0;
				puts(buffer);
				puts("\r\n");
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