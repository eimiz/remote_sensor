#pragma once
#include <stdint.h>
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


