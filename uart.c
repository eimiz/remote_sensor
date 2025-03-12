#include <stdint.h>
#include <string.h>
#include "uart.h"
#include "gpio.h"
#include "nvic.h"
#define TX_PIN  2
#define TX_MODE_BITPOS  (TX_PIN*4)
#define TX_CNF_BITPOS  (TX_MODE_BITPOS + 2)

USART_TypeDef *UART3 = (USART_TypeDef *)(0x40004800);
USART_TypeDef *UART2 = (USART_TypeDef *)(0x40004400);
static uint8_t current = 0;

static void initTxGpio() {
    gpioEnable(&GPIOA, TX_PIN, GPIO_OUT_APP);
}

void uartEnableNVICint() {
    //usart2 irq is 38
    nvicEnableIRQ(38);
}

void uartEnableInt() {
    UART2->CR1 |= (1 << RXNEIE);
}

void uartDisableInt() {
    UART2->CR1 &= ~(1 << RXNEIE);
}

void uartInit() {
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
    UART2->BRR = 9 | (19 << 4);
    //19200 36MHz = 117.1875
    //UART2->BRR = (117 << 4) | 3;

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

void uartSend(uint8_t data) {
    UART2->DR = data;
}


void uartSendBuf(const char *lbuf, int len) {
    for (int i = 0; i < len; i++) {
        while (!(UART2->SR & (1 << 7))) { (void)0;};
        uartSend(lbuf[i]);
    }
        while (!(UART2->SR & (1 << 7))) { (void)0;};
}

void uartSendStr(const char *lbuf) {
    uartSendBuf(lbuf, strlen(lbuf));
}

void uartSendLog(const char *lbuf) {
	uartSendStr("\r\n");
	uartSendStr(lbuf);
	uartSendStr("\r\n");
}
