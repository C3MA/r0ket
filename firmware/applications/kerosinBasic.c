#include <sysinit.h>
#include "basic/basic.h"
#include "lcd/render.h"
#include "core/pmu/pmu.h"

#include "core/dmx/dmx.h"

#include "systick/systick.h" /* needed for active waiting */


void main_kerosinBasic(void) {

	uint8_t enterCnt = 0;
	uint8_t buffer[64];
	
//	cpuInit();                                // Configure the CPU already done in the startup-function
	systickInit(CFG_SYSTICK_DELAY_IN_MS);     // Start systick timer
	gpioInit();                               // Enable GPIO
//	pmuInit();                                // Configure power management
	adcInit();                                // Config adc pins to save power
		
	
	uint8_t* channelBuffer;
	dmx_getDMXbuffer(&channelBuffer);
	channelBuffer[0] = 0xFF; // red
	channelBuffer[1] = 0x00; // green
	channelBuffer[2] = 0x00; // blue
	channelBuffer[3] = 0x00; // empty
	
	dmx_init();
	dmx_start();
	/* ---------------------------------------------- */

	while (1) {
		
	}
	
	dmx_stop();
}
