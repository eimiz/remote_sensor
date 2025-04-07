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
    uint8_t odr = 0;
    switch (dir) {
        case GPIO_IN:
            mode = 0;
            cnf = 1;
            break;
        case GPIO_OUT:
            mode = 1;
            cnf = 0;
            break;
        case GPIO_OUT_APP:
            mode = 0b11;
            cnf = 0b10;
            break;
        case GPIO_IN_PUP:
            mode = 0;
            cnf = 0b11;
            odr = 1 << pin;
            break;
        default:
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
    uint32_t *pGPIO_ODR = (uint32_t *)(&(gpio->gpioRegs->ODR));
    *pGPIO_ODR &= ~(1 << pin);
    *pGPIO_ODR |= odr;
}

void gpioOff(GPIO_Type *gpio, uint8_t pin) {
    gpio->gpioRegs->ODR &= ~( 1 << pin);
}

void gpioOn(GPIO_Type *gpio, uint8_t pin) {
    gpio->gpioRegs->ODR |= ( 1 << pin);
}

bool gpioState(GPIO_Type *gpio, uint8_t pin) {
    return gpio->gpioRegs->IDR & ( 1 << pin);
}
