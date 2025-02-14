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

uint8_t buffer[200] = {0};
uint8_t rxbuffer[3] = {"***"};
uint16_t bpos = 0;
volatile static bool receivedData = false;
volatile static int gotTimerInt = false;
int32_t dmaIntCounter = 0;
uint16_t timerIntCounter = 0;
int ledpos = 1;
char buftmp[20];
const static  uint32_t regads[] = {0, 0x04, 0x08, 0x0c, 0x10, 0x14,
		0x18, 0x1c, 0x20, 0x24, 0x28, 0x2c, 0x34, 0x38, 0x3c,
		0x40, 0x48, 0x4c

};
static int sendCounter = 10;
void TIM2_IRQHandler() {
    if (ledpos++ %2 == 0) led_on();
    else led_off();
    





//    timer_disableInt();
    uint32_t sr = timer_getSR();
    if ((sr % 2) > 0) {
        timerIntCounter++;
    }



//    sprintf((char *)buffer, "(%i)", timerIntCounter);
    uint32_t reg = *(uint32_t *)(TIMER_BASE + 0x34);

//    sprintf((char *)buffer + 7, "[%li]", reg);
    timer_clearInt();
    gotTimerInt = 1;
}

void USART2_IRQHandler() {
    receivedData = true;
    disableUartInt();
}

void DMA1_Channel6_IRQHandler() {
//    memcpy(buffer + 9, rxbuffer, sizeof(rxbuffer));
    clearDmaIntFlag();
    //disableDmaInt();
    dmaIntCounter++;
    //eitoa(buffer, dmaIntCounter);

    memcpy(buffer + sizeof(buffer) - 7, rxbuffer, sizeof(rxbuffer));
//    sprintf((char *)buffer, "%li", dmaIntCounter);


    //buffer[0]='r';
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
  //  buffer[0] = val & 0xff;
}

void loop() {
delay(1000);
  if (receivedData) {
    readRxData();
    enableUartInt();
    receivedData = false;
  }

  if (gotTimerInt > 0) {
    //  timer_enableInt();
   //   timer_enableInt();
      gotTimerInt--;
  }
  
 if (sendCounter-- > 0) {
  sendSomething();
 }

}

int main(void) {
  delay(10);
  setup();
  led_off();
  enableUartInt();
  timer_enableInt();
  timer_start();
    sprintf((char *)buffer, "****>");
	for (int i = 0; i < sizeof(regads) / sizeof(regads[0]); i++) {
		uint32_t *p = (uint32_t*)(0x40000000 + regads[i]);
		sprintf((char *)buffer + (i + 1) * 10, "%x:%li,", regads[i], *p);
	}

    sprintf((char *)buffer + sizeof(buffer) - 3, "\r\n");


  for(;;) {
    loop();
  }
}
