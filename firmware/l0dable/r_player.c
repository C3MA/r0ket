#include <sysinit.h>

#include "basic/basic.h"
#include "basic/byteorder.h"

#include "lcd/lcd.h"
#include "lcd/print.h"

#include "funk/nrf24l01p.h"

//includes useful for r_game:
//#include "usbcdc/usb.h"
//#include "usbcdc/usbcore.h"
//#include "usbcdc/usbhw.h"
//#include "usbcdc/cdcuser.h"
//#include "usbcdc/cdc_buf.h"
//#include "usbcdc/util.h"

#include <string.h>
#include "basic/random.h"

#include "usetable.h"

#define REMOTE_CHANNEL 81

//mac that the player receives
#define PLAYER_MAC     "\x1\x2\x3\x2\x1"

//mac that the game receives
#define GAME_MAC     "\x1\x2\x3\x2\x1"

//#if CFG_USBMSC
//#error "MSC is defined"
//#endif

//#if !CFG_USBCDC
//#error "CDC is not defined"
//#endif

struct NRF_CFG config = {        //for some reason this has to be global
        .channel= REMOTE_CHANNEL,
        .txmac= GAME_MAC,
        .nrmacs=1,
        .mac0=  PLAYER_MAC,
        .maclen ="\x20",
    };

struct packet{
    uint8_t len;
    uint8_t protocol;
    uint8_t command;
    uint32_t id;
    uint32_t ctr;
    
    //union with 19 bytes data
    union content{
        struct button{
            uint8_t button;
            uint8_t reserved[18];
        }button;

        struct text{
            uint8_t x;
            uint8_t y;
            uint8_t flags;
            uint8_t text[16];
        }text;
        struct nick{
            uint8_t flags;
            uint8_t text[18];
        }nick;
        struct nickrequest{
           uint8_t reserved[19];
        }nickrequest;
        struct ack{
           uint8_t flags;
           uint8_t reserved[19];
        }ack;
        struct announce{
           uint8_t gameMac[5];
           uint8_t gameChannel;
           //uint8_t playerMac[5]; playerMac = gameMac+1;
           uint32_t gameId;
           uint8_t gameFlags;
           uint8_t gameTitle[8];
        }announce;
        struct join{
           uint32_t gameId;
        }join;
    }c;
    uint16_t crc;
};

#define FLAGS_MASS_GAME 1

#define FLAGS_ACK_JOINOK    1

#define MASS_ID 1

/**************************************************************************/
/* l0dable for playing games which are announced by other r0kets with the l0dabel r_game */
/* Values of buf[3]:
 * B: packet sent by player, contain information which button is pressed
 * T: packet sent by game, contain text for display
 * N: packet sent by game, requesting nick
 * n: packet sent player, containing nick
 * A: packet sent by game, announcing game
 * J: packet sent by player, requesting to join game
 * a: ack, packet with $ctr was received
 */


uint32_t ctr;
uint32_t id;
uint32_t gameId;

void sendButton(uint8_t button);
void sendJoin(uint32_t game);
void processPacket(struct packet *p);
void processAnnounce(struct announce *a);

uint8_t selectGame();
void playGame();

struct announce games[10];
uint8_t gamecount;

void ram(void)
{
    id = getRandom();
    ctr = 1;
   
    while( selectGame() ){
        playGame();
    }
};

void playGame(void)
{
    int len;
    struct packet p;
 
    while(1){
        uint8_t button = getInputRaw();
        sendButton(button);
        
        while(1){
            len = nrf_rcv_pkt_time(30,sizeof(p),(uint8_t*)&p);
            if(len==sizeof(p)){
            processPacket(&p);
            }else{
                break;
            }
        }
        delayms(20);
    };
}

void showGames(uint8_t selected)
{
    int i;
    lcdClear();
    lcdPrintln("Games:");
    if( gamecount ){
        for(i=0;i<gamecount;i++){
            if( i==selected )
                lcdPrint("*");
            else
                lcdPrint(" ");
            char buf[9];
            memcpy(buf, games[i].gameTitle, 8);
            buf[8] = 0;
            lcdPrintln(buf);
        }
    }else{
        lcdPrint("*No Games");
    }
    lcdRefresh();
}

uint8_t joinGame()
{
    int i;
    for(i=0; i<10; i++){
        struct packet p;
        p.len=sizeof(p); 
        p.protocol='G';
        p.command='J';
        p.id= id;
        p.ctr= ++ctr;
        p.c.join.gameId=gameId;
        nrf_snd_pkt_crc(sizeof(p),(uint8_t*)&p);
        
        int len;
        len = nrf_rcv_pkt_time(30,sizeof(p),(uint8_t*)&p);
        if( len==sizeof(p) ){
            if( (p.len==32) && (p.protocol=='G') && p.command=='a' ){   //check sanity, protocol
                if( p.id == id && p.ctr == ctr ){
                    if( p.c.ack.flags & FLAGS_ACK_JOINOK )
                        return 1;
                    else
                        return 0;
                }
            }
        }
        delayms(70);
    }
    return 0;
}

uint8_t selectGame()
{  
    int len, i, selected;
    struct packet p;

    config.channel = REMOTE_CHANNEL;
    memcpy(config.txmac, GAME_MAC, 5);
    memcpy(config.mac0, PLAYER_MAC, 5);
    nrf_config_set(&config);

    gamecount = 0;
    for(i=0;i<30;i++){
        len= nrf_rcv_pkt_time(30, sizeof(p), (uint8_t*)&p);
        if (len==sizeof(p)){
            processPacket(&p);
        }
    }
    selected = 0;
    while(1){
        showGames(selected);
        int key=getInputWait();
        getInputWaitRelease();
        switch(key){
            case BTN_DOWN:
                if( selected < gamecount-1 ){
                    selected++;
                }
                break;
            case BTN_UP:
                if( selected > 0 ){
                    selected--;
                }
                break;
            case BTN_LEFT:
                return 0;
            case BTN_ENTER:
            case BTN_RIGHT:
                if( gamecount == 0 )
                    return 0;
                gameId = games[selected].gameId;
                memcpy(config.txmac, games[selected].gameMac, 5);
                memcpy(config.mac0, games[selected].gameMac, 5);
                config.mac0[4]++;
                config.channel = games[selected].gameChannel;
                nrf_config_set(&config);
                if( games[selected].gameFlags & FLAGS_MASS_GAME )
                    return 1;
                else
                    return joinGame();
        }
    }
}
   


void processPacket(struct packet *p)
{
   if ((p->len==32) && (p->protocol=='G') && p->id == id){   //check sanity, protocol, id
     if (p->command=='T'){
     //processText(&(p->c.text));
     } 
     else if (p->command=='N'){
     //processNick(&(p->c.nickrequest));
     }
     else if (p->command=='A'){
        processAnnounce(&(p->c.announce));
     }
   }     
}

void processAnnounce(struct announce *a)
{
    if( gamecount < sizeof(games)/sizeof(games[0]) ){
        games[gamecount] = *a;
        gamecount++;
    }
}

//increment ctr and send button state, id and ctr
void sendButton(uint8_t button)
{
    struct packet p;
    p.len=sizeof(p); 
    p.protocol='G'; // Proto
    p.command='B';
    p.id= id;
    p.ctr= ++ctr;
    p.c.button.button=button;

    //lcdClear();
    //lcdPrint("Key:"); lcdPrintInt(buf[2]); lcdNl();

    nrf_snd_pkt_crc(sizeof(p),(uint8_t*)&p);
}

//send join request for game
void sendJoin(uint32_t game)
{
    struct packet p;
    p.len=sizeof(p);
    p.protocol='G';
    p.command='J';
    p.ctr= ++ctr;
    p.id=id;
    p.c.join.gameId=game;

    nrf_snd_pkt_crc(sizeof(p),(uint8_t*)&p);
}
    

/*
void s_init(void){
    usbCDCInit();
    nrf_init();

    struct NRF_CFG config = {
        .channel= REMOTE_CHANNEL,
        .txmac= REMOTE_MAC,
        .nrmacs=1,
        .mac0=  REMOTE_MAC,
        .maclen ="\x10",
    };

    nrf_config_set(&config);
};
*/

/* void process(uint8_t * input){
    __attribute__ ((aligned (4))) uint8_t buf[32];
    puts("process: ");
    puts(input);
    puts("\r\n");
    if(input[0]=='M'){
        buf[0]=0x10; // Length: 16 bytes
        buf[1]='M'; // Proto
        buf[2]=0x01;
        buf[3]=0x01; // Unused

        uint32touint8p(0,buf+4);

        uint32touint8p(0x41424344,buf+8);

        buf[12]=0xff; // salt (0xffff always?)
        buf[13]=0xff;
        nrf_snd_pkt_crc_encr(16,buf,remotekey);
        nrf_rcv_pkt_start();
    };

};
*/

/*
#define INPUTLEN 99
void r_recv(void){
    __attribute__ ((aligned (4))) uint8_t buf[32];
    int len;

    uint8_t input[INPUTLEN+1];
    int inputptr=0;

    nrf_rcv_pkt_start();
    puts("D start");

    getInputWaitRelease();

    while(!getInputRaw()){
        delayms(100);

        // Input
        int l=INPUTLEN-inputptr;
        CDC_OutBufAvailChar (&l);

        if(l>0){
            CDC_RdOutBuf (input+inputptr, &l);
            input[inputptr+l+1]=0;
            for(int i=0;i<l;i++){
                if(input[inputptr+i] =='\r'){
                    input[inputptr+i]=0;
                    process(input);
                    if(i<l)
                        memmove(input,input+inputptr+i+1,l-i);
                    inputptr=-i-1;
                    break;
                };
            };
        };
        inputptr+=l;
        len=nrf_rcv_pkt_poll_dec(sizeof(buf),buf,remotekey);

        // Receive
        if(len<=0){
            delayms(10);
            continue;
        };

        if(buf[1]=='C'){ // Cursor
            puts("C ");
            puts(IntToStrX( buf[2],2 ));
            puts(" ");
            puts(IntToStrX( uint8ptouint32(buf+4), 8 ));
            puts(" ");
            puts(IntToStrX( uint8ptouint32(buf+8), 8 ));
        }else{
            puts("U ");
//          puts("[");puts(IntToStrX(len,2));puts("] ");
            puts(IntToStrX( *(int*)(buf+  0),2 ));
            puts(" ");
            puts(IntToStrX( *(int*)(buf+  1),2 ));
            puts(" ");
            puts(IntToStrX( *(int*)(buf+  2),2 ));
            puts(" ");
            puts(IntToStrX( *(int*)(buf+  3),2 ));
            puts(" ");
            puts(IntToStrX( uint8ptouint32(buf+4),8 ));
            puts(".");
            puts(IntToStrX( uint8ptouint32(buf+8),8 ));
            puts(" ");
            puts(IntToStrX( uint8ptouint32(buf+10),4 ));
        };
        puts("\r\n");

    };

    nrf_rcv_pkt_end();
    puts("D exit");
}
};
*/
