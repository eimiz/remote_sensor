#pragma once
#include <stdbool.h> 
#define AFIO_BASE 0x40010000
#define INT_BASE 0x40010400
void motionInit(int pin);
void motionClearInt();
bool motionPresent();
void motionReset();
