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
char buftmp[20];
uint32_t events;
void measureTempr();
void measureVoltage();

typedef enum {TEMPR_EVENT = 0, MOTION_EVENT, CHECKCHARGE_EVENT} TEvent;
typedef void (*TaskFunc)(void);
typedef struct {
    TEvent event;
    TaskFunc func;
    uint32_t period;
    uint32_t lastTick;
} Task;
static uint32_t ticks = 0;

Task tasks[] = {{TEMPR_EVENT, measureTempr, 2000, 0}, 
    {MOTION_EVENT, measureVoltage, 1300, 0}};


void measureTempr() {

}

void measureVoltage() {

}

void processEvents() {
    for (int i = 0; i < ALEN(tasks); i++) {
        Task *t = &tasks[i];
        if (ticks - t->lastTick >= t->period) {
            t->lastTick = ticks;
            events |= 1 << t->event;
        }
    }
}

void TIM2_IRQHandler() {
//    processEvents();
    if (ledpos++ %2 == 0) led_on();
    else led_off();
    timerClearInt();

}

void USART2_IRQHandler() {
    ticks++;
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

void sendSomething() {
    uint32_t *pSR = (uint32_t*)UART_SR;
    for (int i = 0; i < sizeof(buffer) - 1; i++) {
        sendData1(buffer[i]);
        while (!(*pSR &(1 << 7))) { (void)0;};
    }
}

void setup() {
  led_enable();
  initUart();
  enableUartNVICint();
  initDma();
  timerInit(200, 4000);
  receiveUsartDma(rxbuffer, sizeof(rxbuffer));
}

void readRxData() {
    uint32_t *pDR = (uint32_t *)UART_DR;
    const uint32_t val = *pDR;
}

void loop() {
    delay(1000);
    if (receivedData) {
        readRxData();
        enableUartInt();
        receivedData = false;
    }

  
  sendSomething();
}

int main(void) {
  delay(10);
  setup();
  led_off();
  enableUartInt();
  timerEnableInt();
  timerStart();

  for(;;) {
    loop();
  }
}
