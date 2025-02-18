#include "gpio.h"



GPIO_Type GPIOA={2, (GPIO_TypeDef * )(0x40010800)};
GPIO_Type GPIOB={3, (GPIO_TypeDef * )(0x40010C00)};
GPIO_Type GPIOD={5, (GPIO_TypeDef * )(0x40010C00)};

void gpioEnableClock(GPIO_Type *gpio) {
  uint32_t *pRCC_APB2ENR = (uint32_t *)RCC_APB2ENR;
  // RCC_APB2ENR: Set IOPC_EN 1: :I/O port C clock enabled
  *pRCC_APB2ENR |= ( 1 << gpio->clockpin );
}
void gpioEnable(GPIO_Type *gpio, uint8_t pin, GpioDirection dir) {
  uint8_t cnf;
  uint8_t mode;
  if (dir == GPIO_IN) {
      cnf = 1;
      mode = 0;
  } else {
      cnf = 0;
      mode = 1;
  }

  const uint8_t mode_bp = (pin % 8) * 4;
  const uint8_t cnf_bp = mode_bp + 2;

  uint32_t *pGPIOC_CR = (uint32_t *)(&(gpio->gpioRegs->CRL) + (pin / 8) );

  // CRH: Set to 00: General purpose output push-pull
  uint32_t out = *pGPIOC_CR;
  out &= ~( 0b11 << cnf_bp );  // clear bits
  out |= ( cnf << cnf_bp );  // clear bits


  // MODE: Set to 01: Output mode, max speed 10 MHz.
  out &= ~( 0b11 << mode_bp );  // clear bits
  out |= ( mode << mode_bp );   // set the new value
  *pGPIOC_CR = out;
}

void gpioOff(GPIO_Type *gpio, uint8_t pin) {
  gpio->gpioRegs->ODR &= ~( 1 << pin);
}

void gpioOn(GPIO_Type *gpio, uint8_t pin) {
  gpio->gpioRegs->ODR |= ( 1 << pin);
}


