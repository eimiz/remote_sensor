#include <string.h>
#include <stdlib.h>
#include "delay.h"
#include "led.h"
#include "uart.h"
const uint8_t buffer[] = {"efgh"};
uint16_t bpos = 0;
void sendSomething() {
    sendData1(buffer[bpos++]);
    if (bpos >= sizeof(buffer) - 1) {
        bpos = 0;
    }
}



void setup() {
  led_enable();
  initUart();
}

void loop() {
  led_on();
  delay(200);
  led_off();
  delay(50);

/*  led_on();
  delay(250);
  led_off();
  delay(250);

  led_on();
  delay(125);
  led_off();
  delay(125);
  */

  sendSomething();



  
}

int main(void) {
  delay(1000);
  setup();
  for(;;) {
    loop();
  }
}
