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

#include "core/dmx/dmx.h"

#include "systick/systick.h" /* needed for active waiting */

static void extractOlpeV2(uint8_t* inputbuffer, uint8_t bufferSize ,uint8_t* dmxUniverse, uint8_t* complete/*flag*/);

void main_kerosin(void) {

	uint8_t enterCnt = 0;
	uint8_t buffer[64];
	
//	cpuInit();                                // Configure the CPU already done in the startup-function
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
	
	puts("--- DMX first test ---\r\n");

	
	DoString(10, 25, "Enter:");
	lcdDisplay();
	
	uint8_t* channelBuffer;
	dmx_getDMXbuffer(&channelBuffer);
	
	dmx_start();
	DoString(1, 50, "DMX started");
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
				
				switch (enterCnt % 4)
				{
					case 1:
						DoString(1, 40, "RED              ");
						channelBuffer[0] = 0xFF; // red
						channelBuffer[1] = 0x00; // green
						channelBuffer[2] = 0x00; // blue
						channelBuffer[3] = 0x00; // empty
						break;
					case 2:
						DoString(1, 40, "GREEN              ");
						channelBuffer[0] = 0x00; // red
						channelBuffer[1] = 0xFF; // green
						channelBuffer[2] = 0x00; // blue
						channelBuffer[3] = 0x00; // empty
						break;
					case 3:
						DoString(1, 40, "BLUE              ");
						channelBuffer[0] = 0x00; // red
						channelBuffer[1] = 0x00; // green
						channelBuffer[2] = 0xFF; // blue
						channelBuffer[3] = 0xFF; // empty
						break;						
					case 0:
						DoString(1, 40, "MIXED              ");
						channelBuffer[0] = 0xAA; // red
						channelBuffer[1] = 0x00; // green
						channelBuffer[2] = 0xFF; // blue
						channelBuffer[3] = 0xFF; // empty
						break;						
				}
				
				DoInt(50, 25, (int) (enterCnt));
				lcdDisplay();				
				break;
			case BTN_LEFT:
				readData = CDC_GetInputBuffer(buffer, sizeof(buffer));
				uint8_t dmxUniverseDummy[10];
				memset(dmxUniverseDummy, 0, 10);
				uint8_t readyFlag = 0;
				extractOlpeV2(buffer, readData, dmxUniverseDummy, &readyFlag);
				DoString(1, 40, "RX                  ");
				DoString(1, 40, dmxUniverseDummy);
				DoString(1, 50, "RX-Cnt:     ");
				DoInt(60, 50, readData);
				lcdDisplay();
				break;
			case BTN_DOWN:
				systickInit(1);
				dmx_setLightBox(0, 0, 0, 0);
				systickDelay(500);             // Wait 500ms
				for (int red=0; red < 255; red+=5) {
					dmx_setLightBox(0, red, 0, 0);
					systickDelay(200);             // Wait 200ms
				}
				break;
			case BTN_UP:
				dmx_setChannel(0,33);
				dmx_setChannel(1, 33);
				dmx_setChannel(2, 33);
				break;
			default:
				break;
		}		
	}
	
	dmx_stop();
}

static void extractOlpeV2(uint8_t* inputbuffer, uint8_t bufferSize,
						  uint8_t* dmxUniverse, uint8_t* complete/*flag*/)
{
	int i = 0;
	static int value = -1; //value
	static int channel = -1; //channel
	(*complete) = 0; /* set the flag, that the universe is not complete */
	
	for (i=0; i < bufferSize; i++) {
		if (inputbuffer[i] == 'o' && ((i + 1) < bufferSize) && inputbuffer[i + 1] == 'p')
		{
			channel = 1; /* light people start counting with 1 */
		}
		/* when a signal was found */
		if (channel > 0)
		{
			if (inputbuffer[i] == 'o' && ((i + 1) < bufferSize) && inputbuffer[i + 1] == 'o')
			{
				dmxUniverse[channel] = 'o';
			} 
			else if (inputbuffer[i] != 'o') 
			{
				dmxUniverse[channel] = inputbuffer[i];
			} 
			else 
			{
				channel = -1;
				continue;
			}
			channel++; /* have a look at the next byte */
		}
		
		if (channel > 512)
		{
			channel = -1;
			(*complete) = 1;
		}
	}	
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