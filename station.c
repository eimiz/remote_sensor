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
void TIM2_IRQHandler() {
    if (ledpos++ %2 == 0) led_on();
    else led_off();
    timer_clearInt();
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

void sendSomething() {
    for (int i = 0; i < sizeof(buffer) - 1; i++) {
        sendData1(buffer[i]);
        delay(1);
    }
}

void setup() {
  led_enable();
  initUart();
  enableUartNVICint();
  initDma();
  timer_init(500, 10000);
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
  timer_enableInt();
  timer_start();

  for(;;) {
    loop();
  }
}
