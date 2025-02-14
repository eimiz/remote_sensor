#include "timer.h"
void timer_clearInt() {
    uint32_t *pSR = (uint32_t *)TIMER_SR;
    *pSR &= ~(1 << TIMER_UIF);
}
void timer_init(uint16_t presc, uint16_t period) {
    //enable TIM2 peripheral clock
    uint32_t *pAPB1 = (uint32_t *)UART_APB1ENR;
    *pAPB1 |= (1 << 0);

    uint32_t *pCR1 = (uint32_t *)TIMER_CR1;
    *pCR1 = 0;
    uint32_t *pCR2 = (uint32_t *)TIMER_CR2;
    *pCR2 = 0;

    uint32_t *pSC = (uint32_t *)TIMER_PSC;
    *pSC = presc;

    //reload register
    uint32_t *pArr = (uint32_t *)TIMER_ARR;
    *pArr = period;


    //event generation flags
    uint32_t *pEGR = (uint32_t *)TIMER_EGR;
    *pEGR |=  (1 << TIMER_UG);

    timer_clearInt();


    //enable NVIC interrupt
    uint32_t *pISER = (uint32_t *)(0xE000E100);
    //TIM2 global interrupt
    pISER[0] |= 1 << 28;
}

void timer_start() {
    uint32_t *pCR1 = (uint32_t *)TIMER_CR1;
    *pCR1 |= 1 << TIMER_CEN;
}

void timer_enableInt() {    
    uint32_t *pTIMDIER = (uint32_t *)TIMER_TIM_DIER;
    //enable timer interrupt
    *pTIMDIER |= 1 << TIMER_UIE;
}

void timer_disableInt() {    
    uint32_t *pTIMDIER = (uint32_t *)TIMER_TIM_DIER;
    //enable timer interrupt
    *pTIMDIER &= ~(1 << TIMER_UIE);
}

uint16_t timer_getCounter() {
    uint32_t *pCnt = (uint32_t *)TIMER_CNT;
    return *pCnt;
}

uint32_t timer_getSR() {
    uint32_t *pSR = (uint32_t *)TIMER_SR;
    return *pSR;
}
