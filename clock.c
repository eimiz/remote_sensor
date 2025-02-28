#include <stdint.h>
#include "clock.h"
#include "uart.h"
#define RCC_BASE 0x40021000
#define __IO volatile
typedef struct
{
  __IO uint32_t CR;
  __IO uint32_t CFGR;
  __IO uint32_t CIR;
  __IO uint32_t APB2RSTR;
  __IO uint32_t APB1RSTR;
  __IO uint32_t AHBENR;
  __IO uint32_t APB2ENR;
  __IO uint32_t APB1ENR;
  __IO uint32_t BDCR;
  __IO uint32_t CSR;
} RCC_TypeDef;
#define RCC  ((RCC_TypeDef *)RCC_BASE)

typedef struct
{
  __IO uint32_t ACR;
  __IO uint32_t KEYR;
  __IO uint32_t OPTKEYR;
  __IO uint32_t SR;
  __IO uint32_t CR;
  __IO uint32_t AR;
  __IO uint32_t RESERVED;
  __IO uint32_t OBR;
  __IO uint32_t WRPR;
} FLASH_TypeDef;


#define FLASH_R_BASE (0x00002000UL + 0x00020000UL + 0x40000000UL)
#define FLASH               ((FLASH_TypeDef *)FLASH_R_BASE)


#define READ_BIT(REG, BIT)    ((REG) & (BIT))

#define FLASH_ACR_LATENCY_Pos               (0U)                               
#define FLASH_ACR_LATENCY_Msk               (0x7UL << FLASH_ACR_LATENCY_Pos)    /*!< 0x00000007 */
#define FLASH_ACR_LATENCY                   FLASH_ACR_LATENCY_Msk              /*!< LATENCY[2:0] bits (Latency) */
#define FLASH_ACR_LATENCY_1                 (0x2UL << FLASH_ACR_LATENCY_Pos)    /*!< 0x00000002 */

#define __HAL_FLASH_GET_LATENCY()     (READ_BIT((FLASH->ACR), FLASH_ACR_LATENCY))
#define __HAL_FLASH_SET_LATENCY(__LATENCY__)    (FLASH->ACR = (FLASH->ACR&(~FLASH_ACR_LATENCY)) | (__LATENCY__))


void decreaseFlashLatency(uint32_t FLatency) {
  /* Increasing the number of wait states because of higher CPU frequency */
  if (FLatency < __HAL_FLASH_GET_LATENCY())
  {
    /* Program the new number of wait states to the LATENCY bits in the FLASH_ACR register */
    __HAL_FLASH_SET_LATENCY(FLatency);

    /* Check that the new number of wait states is taken into account to access the Flash
    memory by reading the FLASH_ACR register */
    if (__HAL_FLASH_GET_LATENCY() != FLatency)
  {
  }
}
}

void increaseFlashLatency(uint32_t FLatency) {
  /* Decreasing the number of wait states because of lower CPU frequency */
  if (FLatency > __HAL_FLASH_GET_LATENCY())
  {
    /* Program the new number of wait states to the LATENCY bits in the FLASH_ACR register */
    __HAL_FLASH_SET_LATENCY(FLatency);

    /* Check that the new number of wait states is taken into account to access the Flash
    memory by reading the FLASH_ACR register */
    if (__HAL_FLASH_GET_LATENCY() != FLatency)
  {
  }
}

}

void modifyReg(volatile uint32_t *p, uint32_t msk, uint32_t val) {
	uint32_t reg = *p;
    reg &= ~(msk);
    reg |= val;
    *p = reg;
}

void setApbPrescSafe() {
    modifyReg(&RCC->CFGR, (7 << 8) | (7 << 11), 7 << 8 | 7 << 11);
}

void setApbPresc() {
    modifyReg(&RCC->CFGR, (7 << 8) | (7 << 11), (0b100 << 8) | (0 << 11));
}


void enablePwrEnr() {
    uint32_t *pAPB1 = (uint32_t *)UART_APB1ENR;
    *pAPB1 |= (1 << 28);
}



void clockConfig() {
    //enable pwr enr
    enablePwrEnr();
    //hse on
    RCC->CR |= 1 << 16;
    /* Wait till HSE is ready */
    while ((RCC->CR & (1 << 17)) == 0)
    {};
	//pass

    //set pll source and divider
    modifyReg(&RCC->CFGR, (1<< 16) | (0xf << 18), (1<< 16) | (0b111 << 18) );


    //pll on
//    RCC->CR |= 1 << 24;
    uint32_t *bitReg = (uint32_t *)(0x42000000UL + (0x00020000UL + 0x00001000UL) * 32U + (24U * 4U));
    *bitReg = 1;

   /* Wait till PLL is ready */
   
        while ((RCC->CR & (1 << 25))  == 0)
        {
        }
	//pass






    //APB2 prescaler by default is 1, leave it at that

	increaseFlashLatency(0x2UL);


    //AHB prescaler (HCLK clock divider) default is 1, leave it at that
//	RCC->CFGR |= 0b1000 << 4;



    setApbPrescSafe();
	//apb1 prescaler screwed

    //Switch system clock to PLL
    modifyReg(&RCC->CFGR, 0b11,  0b10);

    
    //wait for switch

//    setApbPresc();
    while ((RCC->CFGR & (0b11 << 2)) != (0b10 << 2)) {
    }

	decreaseFlashLatency(0x02UL);

    setApbPresc();
}
