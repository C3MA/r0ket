#include <sysinit.h>
#include "core/dmx/dmx.h"
#include "basic/basic.h"

volatile unsigned int lastTick;
#include "core/usbcdc/usb.h"
#include "core/usbcdc/usbcore.h"
#include "core/usbcdc/usbhw.h"
#include "core/usbcdc/cdcuser.h"
#include "core/usbcdc/cdc_buf.h"

uint8_t gCounter = 0;
uint8_t gBuffer[2];

void handleCharsWhileMark(uint8_t* buffer)
{
	uint8_t readData = CDC_GetInputBuffer(gBuffer, sizeof(gBuffer));
}

#include "core/uart/uart.h"

void main_kerosinBasic(void) {
	uint8_t enterCnt = 0;
	
	gpioInit();                               // Enable GPIO		
	
	uint8_t* channelBuffer;
	dmx_getDMXbuffer(&channelBuffer);
	channelBuffer[0] = 0xFF; // red
	channelBuffer[1] = 0x00; // green
	channelBuffer[2] = 0x00; // blue
	channelBuffer[3] = 0x00; // empty
	
	/************* Initialize USB UART *******************/
	systickInit(CFG_SYSTICK_DELAY_IN_MS);     // Start systick timer
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
	/************* End Initialize USB UART *******************/
	
	dmx_init();
	
	dmx_setHandler(handleCharsWhileMark);
	
	dmx_start();
	puts("--- DMX first test ---\r\n");
	/* ---------------------------------------------- */

	while (1) {
		switch (getInput()) {
			case BTN_ENTER:
				enterCnt++;

			switch (enterCnt % 4)
			{
				case 1:
					channelBuffer[0] = 0xFF; // red
					channelBuffer[1] = 0x00; // green
					channelBuffer[2] = 0x00; // blue
					channelBuffer[3] = 0x00; // empty
					break;
				case 2:
					dmx_setLightBox(0, 0x00, 0xFF, 0x00);
					break;
				case 3:
					channelBuffer[0] = 0x00; // red
					channelBuffer[1] = 0x00; // green
					channelBuffer[2] = 0xFF; // blue
					channelBuffer[3] = 0xFF; // empty
					break;						
				case 0:
					channelBuffer[0] = 0xAA; // red
					channelBuffer[1] = 0x00; // green
					channelBuffer[2] = 0xFF; // blue
					channelBuffer[3] = 0xFF; // empty
					break;						
			}
				break;
			case BTN_LEFT:
				break;
			case BTN_DOWN:
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
