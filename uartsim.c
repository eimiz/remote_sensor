#include "gpio.h"
#include "uartsim.h"
#include "nvic.h"
#include "uart.h"

#define TX_PIN  10
#define TX_MODE_BITPOS  (TX_PIN*4)
#define TX_CNF_BITPOS  (TX_MODE_BITPOS + 2)
static void initTxGpio() {
    gpioEnable(&GPIOB, TX_PIN, GPIO_OUT_APP);
}

void uartsimInit() {
     initTxGpio();

    //disable uart
    UART3->CR1 &=  ~(1 << UART_UE_ENABLE);

    //enable UART3 clock
    uint32_t *pENR = (uint32_t *)UART_APB1ENR;
    *pENR |= 1 << 18;

    //set boud rate 
    //12 higher bits main part, 4 lower bits - fraction
    //115200 when pclk1 is 8MHz 4.34 = (8e6 / (16 * 115200))
    //115200 when pclk1 is 24MHz 13.02 = (24e6 / (16 * 115200))
    //115200 when pclk1 is 36MHz 19.53125 = (36e6 / (16 * 115200))
    //*pBRR = 5 | (4 << 4);
    UART3->BRR = 9 | (19 << 4);

    //enable transmitter and receiver
    UART3->CR1 |=   (1 << UART_TE_ENABLE) | (1 << UART_RE_ENABLE);

    //enable uart
    UART3->CR1 |=  (1 << UART_UE_ENABLE);
}

void uartsimEnableNVICint() {
//    uart3 irq is 39
    nvicEnableIRQ(39);
}


void uartsimEnableInt() {
    UART3->CR1 |= (1 << RXNEIE);
}

void uartsimDisableInt() {
    UART3->CR1 &= ~(1 << RXNEIE);
}

void uartsimSend(uint8_t data) {
    UART3->DR = data;
}

uint8_t uartsimRead() {
    return UART3->DR;
}

void uartsimSendBuf(const char *lbuf, int len) {
    for (int i = 0; i < len; i++) {
        while (!(UART3->SR & (1 << 7))) { (void)0;};
        uartsimSend(lbuf[i]);
    }
}

