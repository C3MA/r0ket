#include <sysinit.h>

#include "basic/basic.h"

#include "lcd/lcd.h"

#include "funk/nrf24l01p.h"

#include <string.h>

#include "funk/rftransfer.h"
/**************************************************************************/

void f_init(void){
    nrf_init();
    int dx=0;
    int dy=8;
    dx=DoString(0,dy,"Done."); ;dy+=8;
};

void f_status(void){
    int dx=0;
    int dy=8;
    uint8_t buf[4];

    buf[0]=C_R_REGISTER | R_CONFIG;
    buf[1]=0;
    buf[2]=0;
    buf[3]=0;
    dx=DoString(0,dy,"S:"); 
    dx=DoCharX(dx,dy,buf[0]);
    dx=DoCharX(dx,dy,buf[1]);
    dx=DoCharX(dx,dy,buf[2]);
    dx=DoCharX(dx,dy,buf[3]);
    dy+=8;
    nrf_cmd_rw_long(buf,2);
    dx=DoString(0,dy,"R:");
    dx=DoCharX(dx,dy,buf[0]);
    dx=DoCharX(dx,dy,buf[1]);
    dx=DoCharX(dx,dy,buf[2]);
    dx=DoCharX(dx,dy,buf[3]);
    dy+=8;

    int status=nrf_cmd_status(C_NOP);
    dx=DoString(0,dy,"St:"); DoCharX(dx,dy,status);dy+=8;
};

void f_recv(void){
    int dx=0;
    int dy=8;
    uint8_t buf[32];
    int len;

    len=nrf_rcv_pkt_time(500,sizeof(buf),buf);

    if(len==0){
        dx=DoString(0,dy,"No pkt"); dy+=8;
        return;
    }else if (len ==-1){
        dx=DoString(0,dy,"Pkt too lg"); dy+=8;
        return;
    }else if (len ==-2){
        dx=DoString(0,dy,"No pkt error?"); dy+=8;
        return;
    };
    dx=DoString(0,dy,"Size:"); DoInt(dx,dy,len); dy+=8;
    dx=DoString(0,dy,"1:"); DoIntX(dx,dy,*(int*)(buf+0));dy+=8;
    dx=DoString(0,dy,"2:"); DoIntX(dx,dy,*(int*)(buf+4));dy+=8;
    dx=DoString(0,dy,"3:"); DoIntX(dx,dy,*(int*)(buf+8));dy+=8;
    dx=DoString(0,dy,"4:"); DoIntX(dx,dy,*(int*)(buf+12));dy+=8;

    len=crc16(buf,14);
    dx=DoString(0,dy,"c:"); DoIntX(dx,dy,len);dy+=8;

};

void f_send(void){
    static char ctr=1;
    int dx=0;
    int dy=8;
    uint8_t buf[32];
    int status;
    uint16_t crc;

    buf[0]=0x10; // Length: 16 bytes
    buf[1]=0x17; // Proto - fixed at 0x17?
    buf[2]=0xff; // Flags (0xff)
    buf[3]=0xff; // Send intensity

    buf[4]=0x00; // ctr
    buf[5]=0x00; // ctr
    buf[6]=0x00; // ctr
    buf[7]=ctr++; // ctr

    buf[8]=0x0; // Object id
    buf[9]=0x0;
    buf[10]=0x05;
    buf[11]=0xec;

    buf[12]=0xff; // salt (0xffff always?)
    buf[13]=0xff;

    status=nrf_snd_pkt_crc(14,buf);

    dx=DoString(0,dy,"St:"); DoIntX(dx,dy,status); dy+=8;

};

void gotoISP(void) {
    DoString(0,0,"Enter ISP!");
    lcdDisplay(0);
    ISPandReset(5);
}

void lcd_mirror(void) {
    lcdToggleFlag(LCD_MIRRORX);
};

void adc_check(void) {
    int dx=0;
    int dy=8;
    // Print Voltage
    dx=DoString(0,dy,"Voltage:");
    while ((getInputRaw())==BTN_NONE){
        DoInt(dx,dy,GetVoltage());
        lcdDisplay(0);
    };
    dy+=8;
    dx=DoString(0,dy,"Done.");
};

void f_sendBlock(void)
{
    uint8_t data[] = "hallo welt, das hier ist ein langer string, der"
                   "per funk verschickt werden soll.";
    rftransfer_send(strlen(data), data);
}


/**************************************************************************/

const struct MENU_DEF menu_ISP =    {"Invoke ISP",  &gotoISP};
const struct MENU_DEF menu_init =   {"F Init",   &f_init};
const struct MENU_DEF menu_status = {"F Status", &f_status};
const struct MENU_DEF menu_rcv =    {"F Recv",   &f_recv};
const struct MENU_DEF menu_snd =    {"F Send",   &f_send};
const struct MENU_DEF menu_sndblock={"F Send block", &f_sendBlock};
const struct MENU_DEF menu_mirror = {"Mirror",   &lcd_mirror};
const struct MENU_DEF menu_volt =   {"Akku",   &adc_check};
const struct MENU_DEF menu_nop =    {"---",   NULL};

static menuentry menu[] = {
    &menu_init,
    &menu_status,
    &menu_rcv,
    &menu_snd,
    &menu_sndblock,
    &menu_nop,
    &menu_mirror,
    &menu_volt,
    &menu_ISP,
    NULL,
};

static const struct MENU mainmenu = {"Mainmenu", menu};

void main_funk(void) {

    backlightInit();
    font=&Font_7x8;

    while (1) {
        lcdFill(0); // clear display buffer
        lcdDisplay(0);
        handleMenu(&mainmenu);
        gotoISP();
    }
};

void tick_funk(void){
    static int foo=0;
    static int toggle=0;
	if(foo++>50){
        toggle=1-toggle;
		foo=0;
        gpioSetValue (RB_LED0, toggle); 
	};
    return;
};


