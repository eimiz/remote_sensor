#include "udma.h"
#include "uart.h"

void initDma() {
    //enable global clock
    uint32_t *pAHB = (uint32_t *)RCC_AHB;
    *pAHB |= AHB_DMA1_EN;

    //enable NVIC DMA channel 6 interrupt (16)
    uint32_t *pISER = (uint32_t *)(0xE000E100);
    pISER[0] = 1 << 16;
}

void receiveUsartDma(uint8_t *buffer, uint16_t size) {
    uint32_t *pCPAR = (uint32_t *)DMA_CPAR;
    *pCPAR = UART_DR;

    uint32_t **pCMAR = (uint32_t **)DMA_CMAR;
    *pCMAR = (uint32_t *)buffer;

    uint32_t *pCNDTR = (uint32_t *)DMA_CNDTR;
    *pCNDTR = size;

    uint32_t *pCCR = (uint32_t *)DMA_CCR;
    *pCCR = DMA_MINC | DMA_CIRC | DMA_TCIE;

    //Enable dma on Uart control reg
    uint32_t *pUartCR3 = (uint32_t *)UART_CR3;
    *pUartCR3 |= 1 << 6;

    //activate channel
    *pCCR |= DMA_EN;
}


void disableDmaInt() {
    uint32_t *pCCR = (uint32_t *)DMA_CCR;
    *pCCR &= ~(DMA_TCIE);
}

void clearDmaIntFlag() {
    uint32_t *pIFCR = (uint32_t *)(DMA_IFCR);
    uint32_t tmp = 1 << ((DMA_CHANNEL-1)*4 + 1);
    //flag is cleared by writing 1
    *pIFCR |= tmp;
}
