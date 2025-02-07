#ifndef UDMA_H
#define UDMA_H
#include "uart.h"
#define DMA_BASE 0x40020000
#define DMA_CHANNEL 6
#define DMA_CPAR (DMA_BASE + 0x10 + 20 * (DMA_CHANNEL - 1))
#define DMA_CMAR (DMA_BASE + 0x14 + 20 * (DMA_CHANNEL - 1))
#define DMA_CNDTR (DMA_BASE + 0x0C + 20 * (DMA_CHANNEL - 1))
#define DMA_CCR (DMA_BASE + 0x08 + 20 * (DMA_CHANNEL - 1))
//memory increment mode: enabled
#define DMA_MINC (1 << 7)

//circular mode
#define DMA_CIRC (1 << 5)

//transfer complete interrupt enable
#define DMA_TCIE (1 << 1)

//activate channel
#define DMA_EN (1 << 0)

//for DMA1 global clock
#define RCC_AHB (RCC_BOUNDARY_ADDRESS + 0x14)
#define AHB_DMA1_EN (1 << 0)

//dma interrupt status register
#define DMA_IFCR (DMA_BASE + 0x04)



void initDma();
void receiveUsartDma(uint8_t *buffer, uint16_t size);
void disableDmaInt();

#endif

