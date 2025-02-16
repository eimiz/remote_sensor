#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "delay.h"
#include "led.h"
#include "uart.h"
#include "udma.h"
#include "eutils.h"
#include "timer.h"
#include "lcd.h"

uint8_t buffer[200] = {"Pradzia! "};
uint8_t rxbuffer[3] = {"***"};
uint16_t bpos = 0;
volatile static bool receivedData = false;
int32_t dmaIntCounter = 0;
int ledpos = 1;
int ledpos2 = 1;
int ledpos3 = 1;
uint8_t charPos = 0;
const int blinkpin2 = 6;
const int blinkpin3 = 12;
TLcd lcd;
char buftmp[20];
uint32_t events;
void measureTempr();
void measureVoltage();
void ledBlink();
void ledBlink2();
void ledBlink3();
void lcdProcess();
void sendSomething(const uint8_t *lbuf, int len);

typedef enum {TEMPR_EVENT = 0, MOTION_EVENT, CHECKCHARGE_EVENT, BLINK_EVENT, BLINK2_EVENT, BLINK3_EVENT, LCD_EVENT} TEvent;
typedef void (*TaskFunc)(void);
typedef struct {
   const TEvent event;
   const TaskFunc func;
   const uint32_t period;
    uint32_t lastTick;
} Task;
static uint32_t ticks = 0;

Task tasks[] = {{TEMPR_EVENT, measureTempr, 2000, 0}, 
    {MOTION_EVENT, measureVoltage, 2300, 0},
    {BLINK_EVENT, ledBlink, 300, 0},
    {BLINK2_EVENT, ledBlink2, 154, 0},
    {BLINK3_EVENT, ledBlink3, 120, 0},
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
    if (ledpos++ %2 == 0) led_on(&GPIOB, 5);
    else led_off(&GPIOB, 5);
}

void ledBlink2() {
    if (ledpos2++ %2 == 0) led_on(&GPIOA, blinkpin2);
    else led_off(&GPIOA, blinkpin2);
}

void ledBlink3() {
    if (ledpos3++ %2 == 0) led_on(&GPIOB, blinkpin3);
    else led_off(&GPIOB, blinkpin3);
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

void sendSomething(const uint8_t *lbuf, int len) {
    uint32_t *pSR = (uint32_t*)UART_SR;
    for (int i = 0; i < len; i++) {
        sendData1(lbuf[i]);
        while (!(*pSR &(1 << 7))) { (void)0;};
    }
}

void setup() {
  led_enable(&GPIOB, 5);
  led_enable(&GPIOA, blinkpin2);
  led_enable(&GPIOB, blinkpin3);
  initUart();
  enableUartNVICint();
  initDma();
  timerInit(2, 4000);
  receiveUsartDma(rxbuffer, sizeof(rxbuffer));
}

void dumpAscii() {
        for (uint8_t i = 0; i < 16; i++) {
            lcdWriteText(&lcd, (uint8_t[]){charPos + i}, 1);
        }

        charPos += 16;
        char buf[13]={0};
        sprintf(buf, "[%i - %i] ", charPos - 16, charPos);
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

    lcdWriteRam(&lcd, 0, she);
}

void readRxData() {
    uint32_t *pDR = (uint32_t *)UART_DR;
    const uint32_t val = *pDR;
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
        default:
            lcdWriteText(&lcd, (uint8_t[]){val}, 1);
    }
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
  delay(10);
  setup();
  enableUartInt();
  timerEnableInt();
  timerStart();

  for(;;) {
    loop();
  }
}
