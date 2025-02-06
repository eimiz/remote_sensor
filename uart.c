#include <stdint.h>
#include "uart.h"
#define TX_PIN  2
#define TX_MODE_BITPOS  (TX_PIN*4)
#define TX_CNF_BITPOS  (TX_MODE_BITPOS + 2)

static uint8_t current = 0;

void initTxGpio() {
    //enable A port:
    uint32_t *pAENR = (uint32_t*)(UART_RCC_BOUNDARY_ADDRESS + 0x18);
    *pAENR |= (1 << IOPAEN);

    uint32_t *pGPIOL = (uint32_t *)(GPIO_BOUNDARY_ADDRESS + UART_CRL_OFFSET);
    //output mode, max speed 50mhz (01)
    *pGPIOL &= ~(0b11 << TX_MODE_BITPOS);
    *pGPIOL |= (0b11 << TX_MODE_BITPOS);

    //alternate output push pull (00);
    *pGPIOL &= ~(0b11 << TX_CNF_BITPOS);
    *pGPIOL |= 0b10 << TX_CNF_BITPOS;
}
void enableNVICint() {
    //usart2 irq is 38
    uint32_t *pISER = (uint32_t *)(0xE000E100);
    pISER[1] = 1 << 6;

//    NVIC_EnableIRQ(38);

}

void enableInt() {
     uint32_t *pCR1 = (uint32_t *)UART_CR1;
    //enable uart interrupt
//    *pCR1 |= (1 << TCIE);
    //ready to read
    *pCR1 |= (1 << RXNEIE);
}

void disableInt() {
     uint32_t *pCR1 = (uint32_t *)UART_CR1;
//    *pCR1 &= ~(1 << TCIE);

    *pCR1 &= ~(1 << RXNEIE);
}

void initUart() {
     initTxGpio();

     uint32_t *pCR1 = (uint32_t *)UART_CR1;
    //disable uart
    *pCR1 &=  ~(1 << UART_UE_ENABLE);

    //enable UART2 clock
    uint32_t *pENR = (uint32_t *)UART_APB1ENR;
    *pENR |= 1 << UART_EN_BIT;


    uint32_t *pBRR = (uint32_t*)UART_BRR;
    //set boud rate 
    //12 higher bits main part, 4 lower bits - fraction
    *pBRR = 1 | (50 << 4);

    //enable transmitter and receiver
    *pCR1 |=   (1 << UART_TE_ENABLE) | (1 << UART_RE_ENABLE);


    //enable uart
    *pCR1 |=  (1 << UART_UE_ENABLE);
}

void toggleGpio() {
    uint32_t *pOdr = (uint32_t*)(GPIO_BOUNDARY_ADDRESS + UART_ODR_OFFSET);
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
