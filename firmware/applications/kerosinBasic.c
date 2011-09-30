#include <sysinit.h>
#include "core/dmx/dmx.h"

void main_kerosinBasic(void) {
	gpioInit();                               // Enable GPIO		
	
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
