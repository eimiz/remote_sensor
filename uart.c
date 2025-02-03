#include <stdint.h>
#include "uart.h"
#define TX_PIN  3
#define TX_MODE_BITPOS  (TX_PIN*4)
#define TX_CNF_BITPOS  (TX_MODE_BITPOS + 2)
static uint8_t current = 0;
void initTxGpio() {
    //enable A port:
    uint32_t *pAENR = (uint32_t*)(UART_RCC_BOUNDARY_ADDRESS + 0x18);
    *pAENR |= (1 << IOPAEN);

    uint32_t *pGPIOL = (uint32_t *)(GPIO_BOUNDARY_ADDRESS + UART_CRL_OFFSET);
    //output mode, max speed 10mhz (01)
    *pGPIOL &= ~(11 << TX_MODE_BITPOS);
    *pGPIOL |= (01 << TX_MODE_BITPOS);


    //general purpose output push pull (00);
    *pGPIOL &= ~(11 << TX_CNF_BITPOS);



    //set value to 0
/*    uint32_t *pOdr = (uint32_t*)(GPIO_BOUNDARY_ADDRESS + UART_ODR_OFFSET);
    *pOdr &= ~(1 << TX_PIN);
    */
}

void initUart() {
    initTxGpio();
//    return;
    uint32_t *pBRR = (uint32_t*)UART_BRR;
    //set brr to 39.0625
    //12 higher bits main part, 4 lower bits - fraction
    *pBRR = 1 | (39 << 4);
    //enable clock
//    uint32_t *pCR2 = (uint32_t *)UART_CR2;
//    *pCR2 = 1 << UART_PCLK_BIT;



    //enable UART2 clock
    uint32_t *pENR = (uint32_t *)UART_APB1ENR;
    *pENR |= 1 << UART_EN_BIT;

    uint32_t *pCR1 = (uint32_t *)UART_CR1;
    //enable transmitter and uart
    *pCR1 =  1 << UART_UE_ENABLE;
    *pCR1 |= 1 << UART_TE_ENABLE;
    *pCR1 = (1<<UART_TE_ENABLE) | (1 << UART_UE_ENABLE);

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
    toggleGpio();
    uint32_t *pDR = (uint32_t *)UART_DR;
    *pDR = data;
}
