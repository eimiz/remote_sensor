#pragma once
#include "uart.h"
#define TIMER_BASE 0x40000000
#define TIMER_CR1 (TIMER_BASE)
#define TIMER_CR2 (TIMER_BASE + 0x04)
#define TIMER_TIM_DIER (TIMER_BASE + 0x0C)
#define TIMER_SR (TIMER_BASE + 0x10)
#define TIMER_EGR (TIMER_BASE + 0x14)
#define TIMER_CNT (TIMER_BASE + 0x24)
#define TIMER_PSC (TIMER_BASE + 0x28)
#define TIMER_ARR (TIMER_BASE + 0x2C)

#define TIMER_CEN 0
#define TIMER_TIE 6
#define TIMER_UIE 0
#define TIMER_UG 0
#define TIMER_TG 6
#define TIMER_UIF 0
void timerInit(uint16_t presc, uint16_t period);
void timerStart();
void timerEnableInt();
uint16_t timerGetCounter();
void timerClearInt();
void timerDisableInt();
uint32_t timerGetSR();
