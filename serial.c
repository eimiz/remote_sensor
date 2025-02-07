#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "delay.h"
#include "led.h"
#include "uart.h"
#include "udma.h"

uint8_t buffer[] = {"another "};
uint8_t rxbuffer[3] = {0};
uint16_t bpos = 0;
volatile static bool receivedData = false;

void incBpos() {
    bpos++;
    if (bpos >= sizeof(buffer) - 1) {
        bpos = 0;
    }
}

void USART2_IRQHandler() {
  //  incBPos();
    disableUartInt();
    receivedData = true;
}

void DMA1_Channel6_IRQHandler() {
    memcpy(buffer, rxbuffer, sizeof(rxbuffer));
    clearDmaIntFlag();
}


void sendSomething() {
    sendData1(buffer[bpos]);

}

void setup() {
  led_enable();
  initUart();
  enableUartNVICint();
}

void readRxData() {
    uint32_t *pDR = (uint32_t *)UART_DR;
    const uint32_t val = *pDR;
    buffer[0] = val & 0xff;
}

void loop() {
  delay(200);
  led_off();
  
  delay(400);
  led_on();

  //enableUartInt();
  if (receivedData) {
      readRxData();
      receivedData = false;
  }

  sendSomething();
  incBpos();
}

int main(void) {
  delay(10);
  setup();
  led_off();
  delay(700);
  led_on();
  delay(100);
  led_off();
  delay(100);

  for(;;) {
    loop();
  }
}
