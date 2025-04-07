#include <stdbool.h>
#include "station.h"
#include "gpio.h"

static int counter = 0;
bool ison = false;
#define BUT_PIN 1
#define PGPIO  (&GPIOA)
#define CLICK_THRESH 5
Task buttonTask = {BUTTON_EVENT, stationLedToggle, 0, 0, false};

void ledToggle(void *t);
void buttonInit() {
    counter = 0;
    ison = false;
    gpioEnable(PGPIO, BUT_PIN, GPIO_IN_PUP);
    stationRegisterTask(&buttonTask);
    //gpioOn(PGPIO, BUT_PIN);
}

void buttonProbe() {
    if (!ison) {
        if (!gpioState(PGPIO, BUT_PIN)) {
            if (counter++ > CLICK_THRESH) {
                counter = 0;
                stationRegisterEvent(BUTTON_EVENT);
                ison = true;
                return;
            }
        } else {
            counter = 0;
        }
    } else {
        if (gpioState(PGPIO, BUT_PIN)) {
            ison = false;
        }
    }
}



