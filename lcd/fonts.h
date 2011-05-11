#ifndef __FONTS_H_
#define __FONTS_H_

/* Partially based on original code for the KS0108 by Stephane Rey */
/* Based on code code by Kevin Townsend */

#include <sysdefs.h>

typedef struct {
    const uint8_t widthBits; // width, in bits (or pixels), of the character
    const uint16_t offset;   // offset of the character's bitmap, in bytes,
                             // into the the FONT_INFO's data array
} FONT_CHAR_INFO;


struct FONT_DEF {
    uint8_t u8Width;                /* Character width for storage          */
    uint8_t u8Height;               /* Character height for storage         */
    uint8_t u8FirstChar;            /* The first character available        */
    uint8_t u8LastChar;             /* The last character available         */
    const uint8_t *au8FontTable;    /* Font table start address in memory   */
    const FONT_CHAR_INFO *charInfo; /* Pointer to array of char information */
};

typedef struct FONT_DEF * FONT;

#endif
