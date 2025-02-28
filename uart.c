#pragma once
#include <stdint.h>
#include "uart.h"
#include "gpio.h"
#define TX_PIN  2
#define TX_MODE_BITPOS  (TX_PIN*4)
#define TX_CNF_BITPOS  (TX_MODE_BITPOS + 2)

USART_TypeDef *UART3 = (USART_TypeDef *)(0x40004800);
USART_TypeDef *UART2 = (USART_TypeDef *)(0x40004400);
static uint8_t current = 0;

static void initTxGpio() {
    gpioEnable(&GPIOA, TX_PIN, GPIO_OUT_APP);
}

void enableUartNVICint() {
    //usart2 irq is 38
    uint32_t *pISER = (uint32_t *)(0xE000E100);
    pISER[1] = 1 << 6;

//    NVIC_EnableIRQ(38);

}

void enableUartInt() {
     uint32_t *pCR1 = (uint32_t *)UART_CR1;
    //enable uart interrupt
//    *pCR1 |= (1 << TCIE);
    //ready to read
    *pCR1 |= (1 << RXNEIE);
}

void disableUartInt() {
     uint32_t *pCR1 = (uint32_t *)UART_CR1;
//    *pCR1 &= ~(1 << TCIE);

    *pCR1 &= ~(1 << RXNEIE);
}

void initUart() {
     initTxGpio();

    //disable uart
    UART2->CR1 &=  ~(1 << UART_UE_ENABLE);

    //enable UART2 clock
    uint32_t *pENR = (uint32_t *)UART_APB1ENR;
    *pENR |= 1 << UART_EN_BIT;

    //set boud rate 
    //12 higher bits main part, 4 lower bits - fraction
    //115200 when pclk1 is 8MHz 4.34 = (8e6 / (16 * 115200))
    //115200 when pclk1 is 24MHz 13.02 = (24e6 / (16 * 115200))
    //115200 when pclk1 is 36MHz 19.53125 = (36e6 / (16 * 115200))
    //*pBRR = 5 | (4 << 4);
    UART2->BRR = 9 | (19 << 4);

    //enable transmitter and receiver
    UART2->CR1 |=   (1 << UART_TE_ENABLE) | (1 << UART_RE_ENABLE);

    //enable uart
    UART2->CR1 |=  (1 << UART_UE_ENABLE);
}

void toggleGpio() {
    uint32_t *pOdr = (uint32_t*)(GPIOA_BOUNDARY_ADDRESS + UART_ODR_OFFSET);
    if (current == 0) {
        *pOdr &= ~(1 << TX_PIN);
        current = 1;
    } else {
        *pOdr |= 1 << TX_PIN;
        current = 0;
    }
}

void sendData1(uint8_t data) {
   // toggleGpio();
    uint32_t *pDR = (uint32_t *)UART_DR;
    *pDR = data;
}
