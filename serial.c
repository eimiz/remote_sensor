#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "delay.h"
#include "led.h"
#include "uart.h"
#include "udma.h"
#include "eutils.h"

uint8_t buffer[] = {"DI:st329       "};
uint8_t rxbuffer[3] = {"***"};
uint16_t bpos = 0;
volatile static bool receivedData = false;
int32_t dmaIntCounter = 0;
int ledpos = 0;
char buftmp[20];
void USART2_IRQHandler() {
    disableUartInt();
    receivedData = true;
    ledpos++;

}

void DMA1_Channel6_IRQHandler() {
//    memcpy(buffer + 9, rxbuffer, sizeof(rxbuffer));
    clearDmaIntFlag();
    //disableDmaInt();
    dmaIntCounter++;
    //eitoa(buffer, dmaIntCounter);

    memcpy(buffer + sizeof(buffer) - 7, rxbuffer, sizeof(rxbuffer));
    sprintf((char *)buffer, "%li", dmaIntCounter);

    //buffer[0]='r';
}

void sendSomething() {
    for (int i = 0; i < sizeof(buffer) - 1; i++) {
        sendData1(buffer[i]);
        delay(5);
    }
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
delay(1000);
/*led_on();
delay(250);
led_off();
  */
  if (receivedData) {
    readRxData();
    enableUartInt();

    receivedData = false;
    if (ledpos %2 == 0) led_on();
    else led_off();
  }
  

  sendSomething();
}

int main(void) {
  delay(10);
  setup();
  led_off();
  enableUartInt();
  for(;;) {
    loop();
  }
}
