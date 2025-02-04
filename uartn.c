#include "uartn.h"
#define USART_BASE 0x40004400
#define RCC_BASE 0x40021000


void uartnInit() {
	   //enable usart peripheral clock
	    uint32_t *pRCC = (uint32_t *)(RCC_BASE + 0x1C);
	    *pRCC |= (1 << 17);


	    //baud rate
	    uint32_t *pBRR = (uint32_t *)(USART_BASE + 0x08);
	    *pBRR = 50 << 4;

	    //enable clock, optional
	    //uint32_t *pCR2 = (uint32_t *)(USART_BASE + 0x10);
	    //*pCR2 |= 1 << 11;


	    uint32_t *pCR1 = (uint32_t *)(USART_BASE + 0x0c);
	    //enable transmitter and receiver
	    *pCR1 |= (1 << 3) | (1 << 2);


	    //set prescaler
	    uint32_t *pCFGR = (uint32_t *)(RCC_BASE + 0x04);
	    *pCFGR = (0b101 << 8);



	    //enable uart
	    *pCR1 |= (1 << 13);

}



void uartnInitOld() {
    //enable usart peripheral clock
/*    uint32_t *pRCC = (uint32_t *)(RCC_BASE + 0x1C);
    *pRCC |= (1 << 17);
*/

    //baud rate
    uint32_t *pBRR = (uint32_t *)(USART_BASE + 0x08);
    *pBRR = 50 << 4;

    //enable clock, optional
    //uint32_t *pCR2 = (uint32_t *)(USART_BASE + 0x10);
    //*pCR2 |= 1 << 11;


    uint32_t *pCR1 = (uint32_t *)(USART_BASE + 0x0c);
    //enable transmitter and receiver
    *pCR1 |= (1 << 3) | (1 << 2);


    //set prescaler
    uint32_t *pCFGR = (uint32_t *)(RCC_BASE + 0x04);
    *pCFGR = (0b101 << 8);



    //enable uart
    *pCR1 |= (1 << 13);

}

void uartnSend(uint8_t data) {
    uint32_t *pDR = (uint32_t *)(USART_BASE + 0x04);
    *pDR = data;
}

