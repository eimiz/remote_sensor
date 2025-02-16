#include "lcd.h"
#include "led.h"
#include "delay.h"
#include "eutils.h"
void lcdWriteNibble(TLcd *lcd, uint8_t data, int rs);
void lcdWriteByte(TLcd *lcd, uint8_t data, int rs);
void lcdInit(TLcd *lcd, int rs, int clock, int d4, int d5, int d6, int d7) {
    lcd->pos = 0;
    lcd->line = 0;
    lcd->rs = 1 << rs;
    lcd->clock = 1 << clock;
    lcd->d4 = 1 << d4;
    lcd->d5 = 1 << d5;
    lcd->d6 = 1 << d6;
    lcd->d7 = 1 << d7;
    led_enable(&GPIOB, rs);
    led_enable(&GPIOB, clock);
    led_enable(&GPIOB, d4);
    led_enable(&GPIOB, d5);
    led_enable(&GPIOB, d6);
    led_enable(&GPIOB, d7);
    delay(1);
    lcdWriteNibble(lcd, 0b11, 0);
    delay(5);
    lcdWriteNibble(lcd, 0b11, 0);
    delay(1);
    lcdWriteNibble(lcd, 0b11, 0);
    delay(1);
    //set 4 bits in length
    lcdWriteNibble(lcd, 0b10, 0);
    delaymu(20);
    //sets 4 bits in length again, N=1 (2 lines) Font does not matter
    lcdWriteByte(lcd, 0b00101000, 0);
    delaymu(20);
    //off
    lcdWriteByte(lcd, 0b00001000, 0);
    delaymu(20);
    //clear
    lcdWriteByte(lcd, 0b00000001, 0);
    delay(4);
    //entry mode
    lcdWriteByte(lcd, 0b00000110, 0);
    delay(4);
    //on
    lcdWriteByte(lcd, 0b00001100, 0);
    delaymu(20);
    //set start address
    lcdWriteData(lcd, (uint8_t[]){0b10000000}, 1, 0);
    delaymu(20);
}

void lcdClearDataPins(TLcd *lcd) {
    uint32_t out = ~(lcd->d4 | lcd->d5 | lcd->d6 | lcd->d7 | lcd->clock|lcd->rs);
    GPIOB.gpioRegs->ODR &= out;
}

void lcdWriteByte(TLcd *lcd, uint8_t data, int rs) {
    lcdWriteNibble(lcd, data >> 4, rs);
    lcdWriteNibble(lcd, data, rs);
}

void lcdWriteNibble(TLcd *lcd, uint8_t data, int rs) {
    lcdClearDataPins(lcd);
    uint32_t out = lcd->clock;
    if (rs)  out |= lcd->rs;
    if (data & 1) out |= lcd->d4;
    if (data & 2) out |= lcd->d5;
    if (data & 4) out |= lcd->d6;
    if (data & 8) out |= lcd->d7;
    GPIOB.gpioRegs->ODR |= out;
    //hold data min 5ns
    delaymu(3);
    //latch data
    GPIOB.gpioRegs->ODR &= ~(lcd->clock);
    //wait a bit
    delaymu(1);
}

void lcdWriteData(TLcd *lcd, uint8_t *data, int len, int rs) {
    for (int i = 0; i < len; i++) {
        lcdWriteByte(lcd, data[i], rs);
    }
}

void lcdWriteText(TLcd *lcd, uint8_t *data, int len) {
    //reset address to 0
    while (len > 0) {
        int toWrite = MIN(16 - lcd->pos, len);
        lcdWriteData(lcd, data, toWrite, 1);
        data += toWrite;
        lcd->pos += toWrite;
        len -= toWrite;

          if (lcd->pos == 16) {
            lcd->line = (lcd->line + 1) % 2;
            //second row
            lcdWriteData(lcd, (uint8_t[]){0b10000000 | (lcd->line << 6)}, 1, 0);
            lcd->pos = 0;
        }
    }
        //address
        //lcdWriteData(lcd, (uint8_t[]){0b10000000}, 1, 0);
        //lcdWriteData(lcd, data, len, 1);
}

