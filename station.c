#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "clock.h"
#include "delay.h"
#include "gpio.h"
#include "uart.h"
#include "udma.h"
#include "eutils.h"
#include "timer.h"
#include "lcd.h"
#include "wire1.h"

#define WIRE_PIN 0
#define BLINKPIN2 6
#define BLINKPIN3 12
uint8_t buffer[200] = {"Pradzia! "};
uint8_t rxbuffer[3] = {"***"};
uint16_t bpos = 0;
volatile static bool receivedData = false;
int32_t dmaIntCounter = 0;
int ledpos = 1;
int ledpos2 = 1;
int ledpos3 = 1;
uint8_t charPos = 0;

TLcd lcd;
TWire1 wire1;
char buftmp[20];
uint32_t events;
void measureTempr();
void measureVoltage();
void ledBlink();
void ledBlink2();
void ledBlink3();
void lcdProcess();
void sendSomething(const char *lbuf, int len);

typedef enum {TEMPR_EVENT = 0, MOTION_EVENT, CHECKCHARGE_EVENT, BLINK_EVENT, BLINK2_EVENT, BLINK3_EVENT, LCD_EVENT} TEvent;
typedef void (*TaskFunc)(void);
typedef struct {
   const TEvent event;
   const TaskFunc func;
   const uint32_t period;
    uint32_t lastTick;
} Task;
static uint32_t ticks = 0;

Task tasks[] = {{TEMPR_EVENT, measureTempr, 3000, 0}, 
    {MOTION_EVENT, measureVoltage, 4300, 0},
    {BLINK_EVENT, ledBlink, 500, 0},
    {BLINK2_EVENT, ledBlink2, 284, 0},
    {BLINK3_EVENT, ledBlink3, 320, 0},
    {LCD_EVENT, lcdProcess, 0, 0},
    };

void lcdProcess() {
    sendSomething("lcd ", 4);
    lcdInit(&lcd, 6, 7, 15, 14, 13, 9);
    delay(10);
//    const char *txt = "Labas kaip einasi?";
//    lcdWriteText(&lcd, txt, strlen(txt));
    
//    lcdWriteText(&lcd,(uint8_t[]){ 0b10010000, 0b00101101, ' ', 'e'}, 4);

}

void measureTempr() {
    sendSomething("tmpr  ", 6);
}

void measureVoltage() {
    sendSomething("volt  ", 6);
}

void ledBlink() {
    if (ledpos++ %2 == 0) gpioOn(&GPIOB, 5);
    else gpioOff(&GPIOB, 5);
}

void ledBlink2() {
    if (ledpos2++ %2 == 0) gpioOn(&GPIOA, BLINKPIN2);
    else gpioOff(&GPIOA, BLINKPIN2);
}

void ledBlink3() {
    if (ledpos3++ %2 == 0) gpioOn(&GPIOB, BLINKPIN3);
    else gpioOff(&GPIOB, BLINKPIN3);
}

void processEvents() {
    for (int i = 0; i < ALEN(tasks); i++) {
        Task *t = &tasks[i];
        if ((t->period > 0) && (ticks - t->lastTick >= t->period)) {
            t->lastTick = ticks;
            events |= 1 << t->event;
        }

        //if tick counter overflows. Should happen every ~50 days
        if (ticks < t->lastTick) {
            t->lastTick = 0;
        }
    }
}

void TIM2_IRQHandler() {
    ticks++;
    processEvents();
    timerClearInt();
}

void USART2_IRQHandler() {
    receivedData = true;
    disableUartInt();
}

void DMA1_Channel6_IRQHandler() {
    clearDmaIntFlag();
    dmaIntCounter++;
    //eitoa(buffer, dmaIntCounter);

    memcpy(buffer + sizeof(buffer) - 7, rxbuffer, sizeof(rxbuffer));
    sprintf((char *)buffer, "[%li] ", dmaIntCounter);
    const uint8_t mydata[] = {0b10000000};
//    lcdWriteData(&lcd, mydata, sizeof(mydata), 0);
    delaymu(40);
//    lcdWriteText(&lcd, rxbuffer, sizeof(rxbuffer));

}

void sendSomething(const char *lbuf, int len) {
    uint32_t *pSR = (uint32_t*)UART_SR;
    for (int i = 0; i < len; i++) {
        sendData1(lbuf[i]);
        while (!(*pSR &(1 << 7))) { (void)0;};
    }
}

void setup() {
  gpioEnableClock(&GPIOA);
  gpioEnableClock(&GPIOB);
  gpioEnable(&GPIOB, 5, GPIO_OUT);
  gpioEnable(&GPIOA, BLINKPIN2, GPIO_OUT);
  gpioEnable(&GPIOB, BLINKPIN3, GPIO_OUT);
  initUart();
  enableUartNVICint();
  initDma();
  wire1Init(&wire1, &GPIOA, WIRE_PIN);
  timerInit(2, 12000);
  receiveUsartDma(rxbuffer, sizeof(rxbuffer));
}

void dumpAscii() {
        for (uint8_t i = 0; i < 32; i++) {
            lcdWriteText(&lcd, (uint8_t[]){charPos + i}, 1);
        }

        charPos += 32;
        char buf[13]={0};
        sprintf(buf, "[%i - %i] ", charPos - 32, charPos);
        sendSomething(buf, sizeof(buf));
}

void writeDegrees() {
        const uint8_t buf[] = "Dabar 9";
        lcdWriteText(&lcd, buf, sizeof(buf)-1);
        lcdWriteText(&lcd, (uint8_t[]){(uint8_t)223}, 1);
        lcdWriteText(&lcd, (uint8_t[]){(uint8_t)'C'}, 1);
}

void storeChars() {
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

    lcdWriteRam(&lcd, 0, she);
    delay(1);
    lcdWriteRam(&lcd, 1, zhe);
}

void checkTempPresent() {
    if (wire1Reset(&wire1)) {
        sendSomething("present ", 8); 
    } else {
        sendSomething("Not pres ", 9); 
    }
}

void measureTemp() {
    if (wire1MeasureTemp(&wire1) == WIRE1_OK) {
        sendSomething("Meas ok ", 8); 
    } else {
        sendSomething("Meas er ", 8); 
    }
}

void formatTempr(char *buf, uint8_t h, uint8_t l) {
    int lbig = l * 625;
    int lrem = lbig % 100;
    int lrnd = lbig / 100;
    if (lrem >= 50) {
        lrnd += 1;
    }

    sprintf(buf, "%i.%02i", h, lrnd);
}

void readTemp() {
    char buf[30] = {0};
    char buft[20] = {0};
    if (wire1ReadTemp(&wire1) == WIRE1_OK) {
        sendSomething("Read ok ", 8);
        //sprintf(buf, "t=%f", wire1.tempr);
        formatTempr(buft, wire1.tmain, wire1.tfrac);
        sprintf(buf, "Temp is = %s ", buft);
//        lcdHome(&lcd);
        lcdWriteText(&lcd, "abc", 3);
        sendSomething(buf, sizeof(buf));
    } else {
        sendSomething("Read er ", 8); 
    }

    
}

void configTemp() {
    if (wire1Config(&wire1) == WIRE1_OK) {
        sendSomething("Conf ok ", 8); 
    } else {
        sendSomething("Conf er ", 8);
    }
}

void readRxData() {
    uint32_t *pDR = (uint32_t *)UART_DR;
    const uint32_t val = *pDR;
    /*
    if (val == 8) {
        events |= 1 << LCD_EVENT;
    } else {
        dumpAscii();
    }
    return;
    */
   timerDisableInt();
    switch(val) {
        case 8:
            events |= 1 << LCD_EVENT;
            break;
        case 's':
            storeChars();
            sendSomething("Stored ", 7);
            break;
        case 'S':
            lcdWriteText(&lcd, (uint8_t[]){0}, 1);
            break;
        case 'Z':
            lcdWriteText(&lcd, (uint8_t[]){1}, 1);
            break;
        case 'p':
            checkTempPresent();
            break;
        case 'm':
            measureTemp();
            break;
        case 'r':
            readTemp();
            break;
        case 'c':
            configTemp();
            break;
        default:
            lcdWriteText(&lcd, (uint8_t[]){val}, 1);
    }
    timerEnableInt();
}


void checkEvents() {
    for (int i = 0; i < ALEN(tasks); i++) {
        Task *t = &tasks[i];
        if (events & (1 << t->event)) {
            t->func();
            events &= ~(1 << t->event);
        }
    }
}


void loop() {
    delay(1);
    if (receivedData) {
        readRxData();
        enableUartInt();
        receivedData = false;
    }

  checkEvents();
//  sendSomething(buffer, sizeof(buffer) -1);
}

int main(void) {
  clockConfig();
  delay(10);
  setup();
  enableUartInt();
  timerEnableInt();
  timerStart();

  for(;;) {
    loop();
  }
}
