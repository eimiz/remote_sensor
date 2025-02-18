#include "wire1.h"
#include "delay.h"
#define SKIP_ROM 0xCC
#define MEASURE_TEMP 0x44
#define READ_SCRATCH 0xBE
static void doNop() {
    (void)(0);
}

static void sendByte(TWire1 *wire1, uint8_t b) {
    for (int i = 0; i < 8; i++) {
        gpioEnable(wire1->gpio, wire1->pin, GPIO_OUT);
        delaymu(3);
//        gpioOff(wire1->gpio, wire1->pin);
        if (b & 1) {
//            doNop();
            gpioEnable(wire1->gpio, wire1->pin, GPIO_IN);
            delaymu(49);
        } else {
            delaymu(50);
            gpioEnable(wire1->gpio, wire1->pin, GPIO_IN);
        }
        b >>=1;
        delaymu(2);
    }
}

static uint8_t readByte(TWire1 *wire1) {
    uint8_t b = 0;
    for (int i = 0; i < 8; i++) {
        b >>=1;
        gpioEnable(wire1->gpio, wire1->pin, GPIO_OUT);
        delaymu(2);
//        gpioOff(wire1->gpio, wire1->pin);
  //      doNop();
        gpioEnable(wire1->gpio, wire1->pin, GPIO_IN);
        delaymu(3);
        if (wire1->gpio->gpioRegs->IDR & (1 << wire1->pin)) {
            b |= 0x80;
        };

        delaymu(50);
    }


    return b;
}
    

void wire1Init(TWire1 *wire1, GPIO_Type *gpio, int pin) {
    gpioEnableClock(gpio);
    wire1->gpio = gpio;
    wire1->pin = pin;
}

bool wire1Reset(TWire1 *wire1) {
    gpioEnable(wire1->gpio, wire1->pin, GPIO_OUT);
    gpioOff(wire1->gpio, wire1->pin);
    delaymu(500);
//    delay(1);
    gpioEnable(wire1->gpio, wire1->pin, GPIO_IN);
    delaymu(60);
    bool present = wire1->gpio->gpioRegs->IDR & (1 << wire1->pin);
    delaymu(500);
    return present==0;
}

int wire1MeasureTemp(TWire1 *wire1) {
    if (!wire1Reset(wire1)) {
        return WIRE1_ERROR_NOTPRESENT;
    }

    sendByte(wire1, SKIP_ROM);
    sendByte(wire1, MEASURE_TEMP);
    return WIRE1_OK;
}

int wire1ReadTemp(TWire1 *wire1) {
    if (!wire1Reset(wire1)) {
        return WIRE1_ERROR_NOTPRESENT;
    }

    sendByte(wire1, SKIP_ROM);
    sendByte(wire1, READ_SCRATCH);
    uint8_t tempLsb = readByte(wire1);
    uint8_t tempMsb = readByte(wire1);
    wire1->tfrac = tempLsb & 0xf;
    wire1->tmain = (tempMsb << 4) | (tempLsb >> 4);
    wire1Reset(wire1);
    return WIRE1_OK;
}

int wire1Config(TWire1 *wire1) {
    if (!wire1Reset(wire1)) {
        return WIRE1_ERROR_NOTPRESENT;
    }

    sendByte(wire1, SKIP_ROM);
    sendByte(wire1, 0);
    sendByte(wire1, 0);
    //max resolution. Conversion time: 750ms
    sendByte(wire1, 0x7f);
    return WIRE1_OK;
}
