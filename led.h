#ifndef LEG_H
#define LED_H

#include <stdint.h>
#define __IO volatile
typedef struct
{
  __IO uint32_t CRL;
  __IO uint32_t CRH;
  __IO uint32_t IDR;
  __IO uint32_t ODR;
  __IO uint32_t BSRR;
  __IO uint32_t BRR;
  __IO uint32_t LCKR;
} GPIO_TypeDef;

typedef struct {
    int clockpin;
    GPIO_TypeDef *gpioRegs;
} GPIO_Type;


#define RCC_BOUNDARY_ADDRESS 0x40021000
#define RCC_APB2ENR (RCC_BOUNDARY_ADDRESS + 0x18)
extern GPIO_Type GPIOB;
extern GPIO_Type GPIOA;
void led_enable(GPIO_Type *gpio, uint8_t pin);
void led_off(GPIO_Type *gpio, uint8_t pin);
void led_on(GPIO_Type *gpio, uint8_t pin);
#endif
