#include <display.h>
#include <sysdefs.h>
#include "lpc134x.h"
#include "core/ssp/ssp.h"
#include "gpio/gpio.h"
#include "basic/basic.h"
#include "usb/usbmsc.h"

/**************************************************************************/
/* Utility routines to manage nokia display */
/**************************************************************************/

uint8_t lcdBuffer[RESX*RESY_B];
int lcd_layout = 0;
uint32_t intstatus; // Caches USB interrupt state
                    // (need to disable MSC while displaying)

#define TYPE_CMD    0
#define TYPE_DATA   1

static void select() {
#if CFG_USBMSC
    if(usbMSCenabled){
        intstatus=USB_DEVINTEN;
        USB_DEVINTEN=0;
    };
#endif
    /* the LCD requires 9-Bit frames */
    uint32_t configReg = ( SSP_SSP0CR0_DSS_9BIT     // Data size = 9-bit
                  | SSP_SSP0CR0_FRF_SPI             // Frame format = SPI
                  | SSP_SSP0CR0_SCR_8);             // Serial clock rate = 8
    SSP_SSP0CR0 = configReg;
    gpioSetValue(RB_LCD_CS, 0);
}

static void deselect() {
    gpioSetValue(RB_LCD_CS, 1);
    /* reset the bus to 8-Bit frames that everyone else uses */
    uint32_t configReg = ( SSP_SSP0CR0_DSS_8BIT     // Data size = 8-bit
                  | SSP_SSP0CR0_FRF_SPI             // Frame format = SPI
                  | SSP_SSP0CR0_SCR_8);             // Serial clock rate = 8
    SSP_SSP0CR0 = configReg;
#if CFG_USBMSC
    if(usbMSCenabled){
        USB_DEVINTEN=intstatus;
    };
#endif
}

static void lcdWrite(uint8_t cd, uint8_t data) {
    uint16_t frame = 0x0;

    frame = cd << 8;
    frame |= data;

    while ((SSP_SSP0SR & (SSP_SSP0SR_TNF_NOTFULL | SSP_SSP0SR_BSY_BUSY)) != SSP_SSP0SR_TNF_NOTFULL);
    SSP_SSP0DR = frame;
    while ((SSP_SSP0SR & (SSP_SSP0SR_BSY_BUSY|SSP_SSP0SR_RNE_NOTEMPTY)) != SSP_SSP0SR_RNE_NOTEMPTY);
    /* clear the FIFO */
    frame = SSP_SSP0DR;
}

void lcdInit(void) {
    sspInit(0, sspClockPolarity_Low, sspClockPhase_RisingEdge);

    gpioSetValue(RB_LCD_CS, 1);
    gpioSetValue(RB_LCD_RST, 1);

    gpioSetDir(RB_LCD_CS, gpioDirection_Output);
    gpioSetDir(RB_LCD_RST, gpioDirection_Output);

    delayms(100);
    gpioSetValue(RB_LCD_RST, 0);
    delayms(100);
    gpioSetValue(RB_LCD_RST, 1);
    delayms(100);

    select();

    lcdWrite(TYPE_CMD,0xE2);
    delayms(5);
    lcdWrite(TYPE_CMD,0xAF);
    lcdWrite(TYPE_CMD,0xA4);
    lcdWrite(TYPE_CMD,0x2F);
    lcdWrite(TYPE_CMD,0xB0);
    lcdWrite(TYPE_CMD,0x10);
    lcdWrite(TYPE_CMD,0x00);

    uint16_t i;
    for(i=0; i<100; i++)
        lcdWrite(TYPE_DATA,0x00);

    deselect();
}

void lcdFill(char f){
    int x;
    for(x=0;x<RESX*RESY_B;x++) {
        lcdBuffer[x]=f;
    }
};

void lcdSafeSetPixel(char x, char y, bool f){
    if (x>=0 && x<RESX && y>=0 && y < RESY)
        lcdSetPixel(x, y, f);
}

void lcdSetPixel(char x, char y, bool f){
    if (x<0 || x> RESX || y<0 || y > RESY)
        return;
    char y_byte = (RESY-(y+1)) / 8;
    char y_off = (RESY-(y+1)) % 8;
    char byte = lcdBuffer[y_byte*RESX+(RESX-(x+1))];
    if (f) {
        byte |= (1 << y_off);
    } else {
        byte &= ~(1 << y_off);
    }
    lcdBuffer[y_byte*RESX+(RESX-(x+1))] = byte;
}

bool lcdGetPixel(char x, char y){
    char y_byte = (RESY-(y+1)) / 8;
    char y_off = (RESY-(y+1)) % 8;
    char byte = lcdBuffer[y_byte*RESX+(RESX-(x+1))];
    return byte & (1 << y_off);
}

void lcdDisplay(uint32_t shift) {
    char byte;
    select();

    lcdWrite(TYPE_CMD,0xB0);
    lcdWrite(TYPE_CMD,0x10);
    lcdWrite(TYPE_CMD,0x00);
    uint16_t i,page;
    for(page=0; page<RESY_B;page++) {
        for(i=0; i<RESX; i++) {
            if (lcd_layout & LCD_MIRRORX)
                byte=lcdBuffer[page*RESX+RESX-1-((i+shift)%RESX)];
            else
                byte=lcdBuffer[page*RESX+((i+shift)%RESX)];

            if (lcd_layout & LCD_INVERTED)
                byte=~byte;

            lcdWrite(TYPE_DATA,byte);
        }
    }

    deselect();
}

inline void lcdInvert(void) {
    lcdToggleFlag(LCD_INVERTED);
}

void lcdToggleFlag(int flag) {
    lcd_layout=lcd_layout ^ flag;
}
