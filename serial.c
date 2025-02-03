#include <string.h>
#include <stdlib.h>
#include "delay.h"
#include "led.h"
#include "uart.h"


void sendSomething() {
    sendData1('j');
}



void setup() {
  led_enable();
  initUart();
}

void loop() {
  led_on();
  delay(300);
  led_off();
  delay(100);

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
  setup();
  for(;;) {
    loop();
  }
}
