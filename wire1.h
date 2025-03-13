#pragma once
#include <stdbool.h>
#include "gpio.h"
#define WIRE_PIN 0
typedef struct {
    GPIO_Type *gpio;
    int pin;
    uint8_t tfrac;
    uint8_t tmain;

} TWire1;

typedef enum {WIRE1_OK = 0, WIRE1_ERROR_NOTPRESENT} TWIRE_RESULT;

void wire1Init(TWire1 *wire1, GPIO_Type *gpio, int pin);
bool wire1Reset(TWire1 *wire1);
int wire1Config(TWire1 *wire1);
int wire1ReadTemp(TWire1 *wire1);
int wire1MeasureTemp(TWire1 *wire1);
