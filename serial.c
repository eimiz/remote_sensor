#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "delay.h"
#include "led.h"
#include "uart.h"
#include "udma.h"
#include "eutils.h"

uint8_t buffer[] = {"DI:initas        "};
uint8_t rxbuffer[3] = {"***"};
uint16_t bpos = 0;
volatile static bool receivedData = false;
uint16_t dmaIntCounter = 0;
void incBpos() {
    bpos++;
    if (bpos >= sizeof(buffer) - 1) {
        bpos = 0;
        delay(500);
        memcpy(buffer + 9, rxbuffer, sizeof(rxbuffer));
    }
}

void USART2_IRQHandler() {
  //  incBPos();
    disableUartInt();
    receivedData = true;
}

void DMA1_Channel6_IRQHandler() {
//    memcpy(buffer + 9, rxbuffer, sizeof(rxbuffer));
    led_on();
    clearDmaIntFlag();
    //disableDmaInt();
    dmaIntCounter++;
    eitoa(buffer + 3, dmaIntCounter);
    buffer[0]='r';
}


void sendSomething() {
    sendData1(buffer[bpos]);

}

void setup() {
  led_enable();
  initUart();
  enableUartNVICint();
  initDma();
  receiveUsartDma(rxbuffer, sizeof(rxbuffer));
}

void readRxData() {
    uint32_t *pDR = (uint32_t *)UART_DR;
    const uint32_t val = *pDR;
    buffer[0] = val & 0xff;
}

void loop() {
/*  delay(1);
  led_off();
  
  delay(2);
  led_on();
*/
delay(2);
  //enableUartInt();
  /*
  if (receivedData) {
      readRxData();
      receivedData = false;
  }
  */

  sendSomething();
  incBpos();
}

int main(void) {
  delay(10);
  setup();
  led_off();

  for(;;) {
    loop();
  }
}
