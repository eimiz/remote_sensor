#include <stdbool.h>
#include "station.h"
#include "gpio.h"

static int counter = 0;
#define BUT_PIN 1
#define PGPIO  (&GPIOA)
#define CLICK_THRESH 5
Task buttonTask = {BUTTON_EVENT, stationLedToggle, 0, 0, false};

void ledToggle(void *t);
void buttonInit() {
    return;
    counter = 0;
    gpioEnable(PGPIO, BUT_PIN, GPIO_IN_PUP);
    stationRegisterTask(&buttonTask);
}

void buttonProbe() {
    return;
    if (!gpioState(PGPIO, BUT_PIN)) {
        if (counter++ > CLICK_THRESH) {
            counter = 0;
            stationRegisterEvent(BUTTON_EVENT);
            return;
        }
    } else {
        counter = 0;
    }
}



