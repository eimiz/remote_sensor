#include <string.h>
#include <stdlib.h>
#include "delay.h"
#include "led.h"
#include "uart.h"
const uint8_t buffer[] = {"zxyu"};
uint16_t bpos = 0;

void incBPos() {
    bpos++;
    if (bpos >= sizeof(buffer) - 1) {
        bpos = 0;
    }
}

void USART2_IRQHandler() {
    incBPos();
    led_on();
    disableInt();
}


void sendSomething() {
    sendData1(buffer[bpos]);

}

void setup() {
  led_enable();
  initUart();
  enableNVICint();
}

void loop() {
  led_on();
  delay(200);
  led_off();
  
  delay(400);

  enableInt();
  sendSomething();
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
