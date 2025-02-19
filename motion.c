#include "motion.h"
#include "gpio.h"

void motionInit(int pin) {
    //enable AFIO clock (not sure if needed)
    uint32_t *pENR2 = (uint32_t *)RCC_APB2ENR;
    *pENR2 |= 1 << 0;

    //for port A pins nothing to set on pEXTI since 0000 is portA
    //uint32_t *pEXTI = (uint32_t *)AFIO_BASE + 0x08 + (pin / 4) * 4;
    //int shift =  (pin % 4) * 4;
    
    //set interrupt mask;
    uint32_t *pIMR = (uint32_t *)(INT_BASE + 0x00);
    *pIMR |= 1 << pin;

    //set event mask
    uint32_t *pEMR = (uint32_t *)(INT_BASE + 0x04);
    *pEMR |= 1 << pin;

    //enable rising trigger
    uint32_t *pRTSR = (uint32_t *)(INT_BASE + 0x08);
    *pRTSR |= 1 << pin;


     gpioEnable(&GPIOA, pin, GPIO_IN);

     
    //enable NVIC interrupt
    uint32_t *pISER = (uint32_t *)(0xE000E100);
    //EXTI10_15 global interrupt, pos 40
    pISER[1] |= 1 << 8;
}

