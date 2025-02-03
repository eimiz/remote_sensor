#pragma once

#include <stdint.h>

#define GPIO_CRH_OFFSET 0x4
#define GPIO_ODR_OFFSET 0xC

//#define GPIOC_BOUNDARY_ADDRESS 0x40011000
//actually GPIOB (for black board)
#define GPIOC_BOUNDARY_ADDRESS   0x40010C00
//actually gpioA for testing
//#define GPIOC_BOUNDARY_ADDRESS 0x40010800

#define GPIOC_CRH (GPIOC_BOUNDARY_ADDRESS + GPIO_CRH_OFFSET)
#define GPIOC_ODR (GPIOC_BOUNDARY_ADDRESS + GPIO_ODR_OFFSET)

#define RCC_BOUNDARY_ADDRESS 0x40021000
#define RCC_APB2ENR (RCC_BOUNDARY_ADDRESS + 0x18)
//for port c
//#define IOPC_EN 4

//for port b
#define IOPC_EN 3

#define PC13  12
//for pb12
//#define PC13  12

void led_enable(void);
void led_on(void);
void led_off(void);
