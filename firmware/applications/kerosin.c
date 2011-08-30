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

/* Use the 16bit timer for the DMX signal generation */
#include "core/cpu/cpu.h"

void TIMER16_0_IRQHandler(void){
	TMR_TMR16B0IR = TMR_TMR16B0IR_MR0;
	static int time=0;
	if (time==0){time=1;} else {time=0;}
	gpioSetValue (RB_LED2, time);
	gpioSetValue(RB_SPI_SS0, time);
};
void backlightInit(void);


void main_kerosin(void) {

	uint8_t enterCnt = 0;
	uint8_t buffer[64];
	
//	cpuInit();                                // Configure the CPU
	systickInit(CFG_SYSTICK_DELAY_IN_MS);     // Start systick timer
	gpioInit();                               // Enable GPIO
	pmuInit();                                // Configure power management
	adcInit();                                // Config adc pins to save power
	lcdInit();
		
    DoString(2,5,"USB");
	
	gpioSetDir(RB_SPI_SS0, gpioDirection_Output);
	gpioSetDir(RB_LED2, gpioDirection_Output);
	
	/*--- SETUP the 16bit TIMER ---*/
	/* Enable the clock for CT16B0 */
    SCB_SYSAHBCLKCTRL |= (SCB_SYSAHBCLKCTRL_CT16B0);
	TMR_TMR16B0MR0 = (72E6/5E3)/2; //timer einschalten, auf 5kHz(?) setzen 
	
    /* Configure match control register to raise an interrupt and reset on MR0 */
    TMR_TMR16B0MCR = (TMR_TMR16B0MCR_MR0_INT_ENABLED | TMR_TMR16B0MCR_MR0_RESET_ENABLED);
	
    /* Enable the TIMER0 interrupt */
    NVIC_EnableIRQ(TIMER_16_0_IRQn);
	
	/* ENABLE the 16bit timer */
	TMR_TMR16B0TCR = TMR_TMR16B0TCR_COUNTERENABLE_ENABLED;
	DoString(1, 50, "Time enabled!");
	
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