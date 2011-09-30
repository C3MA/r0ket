#include <sysinit.h>
#include "core/dmx/dmx.h"


volatile unsigned int lastTick;
#include "core/usbcdc/usb.h"
#include "core/usbcdc/usbcore.h"
#include "core/usbcdc/usbhw.h"
#include "core/usbcdc/cdcuser.h"
#include "core/usbcdc/cdc_buf.h"



#include "core/uart/uart.h"

void main_kerosinBasic(void) {
	gpioInit();                               // Enable GPIO		
	
	uint8_t* channelBuffer;
	dmx_getDMXbuffer(&channelBuffer);
	channelBuffer[0] = 0xFF; // red
	channelBuffer[1] = 0x00; // green
	channelBuffer[2] = 0x00; // blue
	channelBuffer[3] = 0x00; // empty
	
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
	
	dmx_init();
	dmx_start();
	puts("--- DMX first test ---\r\n");
	/* ---------------------------------------------- */

	while (1) {
		
	}
	
	dmx_stop();
}
