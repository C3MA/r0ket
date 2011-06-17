#include <sysinit.h>

#include "basic/basic.h"

#include "lcd/render.h"
#include "lcd/backlight.h"
#include "lcd/allfonts.h"

#include "usb/usbmsc.h"

void ReinvokeISP(void);

/**************************************************************************/

void module_bsx(void) {
    int yctr = 18;
    int dx = 0;
    uint32_t brightness = 0;

    font_direction = FONT_DIR_LTR; // LeftToRight is the default
    font = &Font_7x8;

    DoString(0, 0, "bsx");
    lcdDisplay(0);
    delayms(1000);
    usbMSCInit();

    while (1) {
        uint32_t results;
        lcdDisplay(0);
        delayms(10);

        if (gpioGetValue(RB_BTN4)==0) {
            while(gpioGetValue(RB_BTN4)==0);
            DoString(0,8,"Enter ISP!");
            lcdDisplay(0);
            ReinvokeISP();
        }
    }

    return;
}

