#pragma once
#include <stdint.h>
//usart2
//usart2: 0x40004400, usart3: 0x40004800
#define UART_BOUNDARY 0x40004400
#define RCC_BOUNDARY_ADDRESS 0x40021000
#define RCC_APB1RSTR_OFFSET 0x10

//MCO clock output

#define RCC_CFGR_OFFSET 0x04
#define RCC_CFGR (RCC_BOUNDARY_ADDRESS + RCC_CFGR_OFFSET)

//GPIOA TX2
#define GPIOA_BOUNDARY_ADDRESS 0x40010800
#define UART_CRL_OFFSET 0x0
#define UART_ODR_OFFSET 0x0C
#define IOPAEN 2

//port clock enable regigster offset
#define UART_APB1ENR_OFFSET  0x1C
#define UART_APB1ENR (RCC_BOUNDARY_ADDRESS + UART_APB1ENR_OFFSET)
#define RCC_APB1RSTR (RCC_BOUNDARY_ADDRESS + RCC_APB1RSTR_OFFSET)


#define UART_SR_OFFSET 0x00
#define UART_DR_OFFSET 0x04
#define UART_BRR_OFFSET 0x08
#define UART_CR1_OFFSET 0x0C
#define UART_CR2_OFFSET 0x10
#define UART_CR3_OFFSET 0x14
#define UART_TE_ENABLE 3
#define UART_RE_ENABLE 2
#define UART_UE_ENABLE 13
#define UART_PCLK_BIT 11
#define __IO volatile

typedef struct
{
  __IO uint32_t SR;         /*!< USART Status register,                   Address offset: 0x00 */
  __IO uint32_t DR;         /*!< USART Data register,                     Address offset: 0x04 */
  __IO uint32_t BRR;        /*!< USART Baud rate register,                Address offset: 0x08 */
  __IO uint32_t CR1;        /*!< USART Control register 1,                Address offset: 0x0C */
  __IO uint32_t CR2;        /*!< USART Control register 2,                Address offset: 0x10 */
  __IO uint32_t CR3;        /*!< USART Control register 3,                Address offset: 0x14 */
  __IO uint32_t GTPR;       /*!< USART Guard time and prescaler register, Address offset: 0x18 */
} USART_TypeDef;


extern USART_TypeDef *UART3;
extern USART_TypeDef *UART2;


//usart2: 17, usart3: 18
#define USART2_RST_BITPOS 17

//usart 2: 17, usart3: 18
#define UART_EN_BIT 17

//USART interrupts
#define TXEIE 7

#define TCIE 6
#define RXNEIE 5

#define UART_DR (UART_BOUNDARY + UART_DR_OFFSET)
#define UART_BRR (UART_BOUNDARY + UART_BRR_OFFSET)
#define UART_SR (UART_BOUNDARY + UART_SR_OFFSET)
#define UART_CR1 (UART_BOUNDARY + UART_CR1_OFFSET)
#define UART_CR2 (UART_BOUNDARY + UART_CR2_OFFSET)
#define UART_CR3 (UART_BOUNDARY + UART_CR3_OFFSET)

void initUart();
void sendData1(uint8_t data);
void enableUartInt();
void disableUartInt();
void enableUartNVICint();
void clearDmaIntFlag();
