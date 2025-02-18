#include "delay.h"

#define DELAY_COUNT_1MS      4000U
#define DELAY_COUNT_1US      3U

// Command: a simple do-nothing delay for approximately `ms` milliseconds
void delay(uint32_t ms) {
  for(uint32_t i = 0 ; i < ms * DELAY_COUNT_1MS ; i++);
}

void delaymu(uint32_t micros) {
  for(uint32_t i = 0 ; i < micros * DELAY_COUNT_1US ; i++);
}
