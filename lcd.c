#include <string.h>
#include "lcd.h"
#include "gpio.h"
#include "delay.h"
#include "eutils.h"
#include "motion.h"
#include "uart.h"
#include "station.h"
const uint8_t she[] = {
    0b00001010,
    0b00000100,
    0b00001111,
    0b00010000,
    0b00001100,
    0b00000010,
    0b00000001,
    0b00011110
};

const uint8_t zhe[] = {
    0b00001010,
    0b00000100,
    0b00011111,
    0b00000001,
    0b00000110,
    0b00001000,
    0b00010000,
    0b00011111
};

const uint8_t deg[] = {
    0b00000111,
    0b00000101,
    0b00000111,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000
};


const uint8_t antenna[] = {
    0b00001110,
    0b00010001,
    0b00000000,
    0b00000100,
    0b00001010,
    0b00000000,
    0b00000000,
    0b00000100
};

void lcdTaskProcess(void *t);
static Task task  = {LCD_EVENT, lcdTaskProcess, 1000, 0, true};
static bool motionDetected = false;

static void lcdWriteNibble(TLcd *lcd, uint8_t data, int rs);
static void lcdWriteByte(TLcd *lcd, uint8_t data, int rs);
void lcdTaskProcess(void *t) {
    if (motionDetected) {
        uartSendLog("motion callback called");
        motionDetected = false;
    }
}

void motionCallback() {
    motionDetected = true;
}

void lcdInit(TLcd *lcd, int rs, int clock, int d4, int d5, int d6, int d7) {
    lcd->pos = 0;
    lcd->line = 0;
    lcd->rs = 1 << rs;
    lcd->clock = 1 << clock;
    lcd->d4 = 1 << d4;
    lcd->d5 = 1 << d5;
    lcd->d6 = 1 << d6;
    lcd->d7 = 1 << d7;
    gpioEnable(&GPIOB, rs, GPIO_OUT);
    gpioEnable(&GPIOB, clock, GPIO_OUT);
    gpioEnable(&GPIOB, d4, GPIO_OUT);
    gpioEnable(&GPIOB, d5, GPIO_OUT);
    gpioEnable(&GPIOB, d6, GPIO_OUT);
    gpioEnable(&GPIOB, d7, GPIO_OUT);
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
    motionAddListener(motionCallback);
    stationStartTask(&task);
}

void lcdHome(TLcd *lcd) {
    lcdWriteData(lcd, (uint8_t[]){0b10000000}, 1, 0);
    lcd->line = 0;
    lcd->pos = 0;
}

void lcdClearDataPins(TLcd *lcd) {
    uint32_t out = ~(lcd->d4 | lcd->d5 | lcd->d6 | lcd->d7 | lcd->clock|lcd->rs);
    GPIOB.gpioRegs->ODR &= out;
}

static void lcdWriteByte(TLcd *lcd, uint8_t data, int rs) {
    lcdWriteNibble(lcd, data >> 4, rs);
    lcdWriteNibble(lcd, data, rs);
}

static void lcdWriteNibble(TLcd *lcd, uint8_t data, int rs) {
    lcdClearDataPins(lcd);
    uint32_t out = lcd->clock;
    if (rs)  out |= lcd->rs;
    if (data & 1) out |= lcd->d4;
    if (data & 2) out |= lcd->d5;
    if (data & 4) out |= lcd->d6;
    if (data & 8) out |= lcd->d7;
    GPIOB.gpioRegs->ODR |= out;
    //hold data min 5ns
    delaymu(10);
    //latch data
    GPIOB.gpioRegs->ODR &= ~(lcd->clock);
    //wait a bit
    delaymu(10);
}

void lcdWriteData(TLcd *lcd, const uint8_t *data, int len, int rs) {
    for (int i = 0; i < len; i++) {
        lcdWriteByte(lcd, data[i], rs);
    }
}

void lcdWriteText(TLcd *lcd, const uint8_t *data, int len) {
    //reset address to 0
    while (len > 0) {
        int toWrite = MIN(16 - lcd->pos, len);
        lcdWriteData(lcd, data, toWrite, 1);
        data += toWrite;
        lcd->pos += toWrite;
        len -= toWrite;

          if (lcd->pos >= 16) {
            lcd->line = (lcd->line + 1) % 2;
            //second row
            lcdWriteData(lcd, (uint8_t[]){0b10000000 | (lcd->line << 6)}, 1, 0);
            lcd->pos = 0;
        }

        delay(2);
    }
        //address
        //lcdWriteData(lcd, (uint8_t[]){0b10000000}, 1, 0);
        //lcdWriteData(lcd, data, len, 1);
}


void lcdWriteRam(TLcd *lcd, const uint8_t addr, const uint8_t *data) {
    lcdWriteByte(lcd, 0b01000000 | (addr & 0x7) << 3, 0);
    for (int i = 0; i < 8; i++) {
        lcdWriteByte(lcd, data[i], 1);
    }
    //set start address
    lcdWriteData(lcd, (uint8_t[]){0b10000000}, 1, 0);
    delaymu(20);
}

static void lcdWriteRow(TLcd *lcd, const char *str, int row) {
    lcdWriteData(lcd, (uint8_t[]){0b10000000 | (row << 6)}, 1, 0);
    delay(1);
    int toWrite = MIN(16, strlen(str));
    lcdWriteData(lcd, str, toWrite, 1);
    int remaining = 16 - toWrite;
    for (int i = 0; i < remaining; i++) {
        lcdWriteData(lcd, " ", 1, 1);
    }
}

void lcdWriteFirstRow(TLcd *lcd, const char *str) {
    lcdWriteRow(lcd, str, 0);
}

void lcdWriteSecondRow(TLcd *lcd, const char *str) {
    lcdWriteRow(lcd, str, 1);
}

void lcdStoreChars(TLcd *lcd) {
    lcdWriteRam(lcd, 0, she);
    delay(1);
    lcdWriteRam(lcd, 1, zhe);
    delay(1);
    lcdWriteRam(lcd, 2, deg);
    delay(1);
    lcdWriteRam(lcd, 3, antenna);
}
