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

uint8_t buffer[200] = {"Pradzia! "};
uint8_t rxbuffer[3] = {"***"};
uint16_t bpos = 0;
volatile static bool receivedData = false;
int32_t dmaIntCounter = 0;
int ledpos = 1;
int ledpos2 = 1;
int ledpos3 = 1;
const int blinkpin2 = 6;
const int blinkpin3 = 12;
char buftmp[20];
uint32_t events;
void measureTempr();
void measureVoltage();
void ledBlink();
void ledBlink2();
void ledBlink3();
void sendSomething(const uint8_t *lbuf, int len);

typedef enum {TEMPR_EVENT = 0, MOTION_EVENT, CHECKCHARGE_EVENT, BLINK_EVENT, BLINK_EVENT2, BLINK_EVENT3} TEvent;
typedef void (*TaskFunc)(void);
typedef struct {
   const TEvent event;
   const TaskFunc func;
   const uint32_t period;
    uint32_t lastTick;
} Task;
static uint32_t ticks = 0;

Task tasks[] = {{TEMPR_EVENT, measureTempr, 2000, 0}, 
    {MOTION_EVENT, measureVoltage, 1300, 0},
    {BLINK_EVENT, ledBlink, 300, 0},
    {BLINK_EVENT2, ledBlink2, 213, 0},
    {BLINK_EVENT3, ledBlink3, 134, 0},
    };


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

void readRxData() {
    uint32_t *pDR = (uint32_t *)UART_DR;
    const uint32_t val = *pDR;
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
