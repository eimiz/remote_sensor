#pragma once
#include <stdint.h>
#define LCD_ANTENNA_CHR 3
#define LCD_ZHE_CHR 1
typedef struct {
    int pos; //position of cursor
    int line; //current line
    int rs;
    int clock;
    int d4;
    int d5;
    int d6;
    int d7;
} TLcd;

void lcdInit(TLcd *lcd, int rs, int clock, int d4, int d5, int d6, int d7);
void lcdWriteData(TLcd *lcd, const uint8_t *data, int len, int rs);
void lcdWriteText(TLcd *lcd, const uint8_t *data, int len);
void lcdWriteRam(TLcd *lcd, const uint8_t addr, const uint8_t *data);
void lcdHome(TLcd *lcd);
void lcdWriteFirstRow(TLcd *lcd, const char *str);
void lcdWriteSecondRow(TLcd *lcd, const char *str);
void lcdStoreChars(TLcd *lcd);
void lcdMotionCallback();
