#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "clock.h"
#include "delay.h"
#include "gpio.h"
#include "uart.h"
#include "udma.h"
#include "eutils.h"
#include "timer.h"
#include "lcd.h"
#include "wire1.h"
#include "motion.h"
#include "uartsim.h"
#include "circbuf.h"
#include "commands.h"

#define WIRE_PIN 0
#define BLINKPIN2 6
#define BLINKPIN3 12
uint8_t buffer[200] = {"Pradzia! "};
uint8_t rxbuffer[3] = {"***"};

uint16_t bpos = 0;
static bool receivedData = false;
static bool receivedSimData = false;
int32_t dmaIntCounter = 0;
static bool passThrough = false;
int ledpos = 1;
int ledpos2 = 1;
int ledpos3 = 1;
uint8_t charPos = 0;
static int extCounter = 0;
int uart3Counter = 0;
int uart2Counter = 0;
TLcd lcd;
TWire1 wire1;
TBuf rxCbuf;
char buftmp[20];
uint32_t events;
void dallasProc();
void measureVoltage();
void ledBlink();
void ledBlink2();
void ledBlink3();
void lcdProcess();
void sendSomething(const char *lbuf, int len);
void storeChars();
void readTemp();
void uartsimProcess();

typedef enum {TEMPR_EVENT = 0, MOTION_EVENT, CHECKCHARGE_EVENT, BLINK_EVENT, BLINK2_EVENT, BLINK3_EVENT, LCD_EVENT, UARTSIM_EVENT} TEvent;
typedef void (*TaskFunc)(void);
typedef struct {
   const TEvent event;
   const TaskFunc func;
   const uint32_t period;
    uint32_t lastTick;
} Task;
static uint32_t ticks = 0;
int tempstatus = 0;

Task tasks[] = {
    {TEMPR_EVENT, dallasProc, 1500, 0},
    {MOTION_EVENT, measureVoltage, 6300, 0},
    {BLINK_EVENT, ledBlink, 500, 0},
    {BLINK2_EVENT, ledBlink2, 284, 0},
    {BLINK3_EVENT, ledBlink3, 320, 0},
//    {UARTSIM_EVENT, uartsimProcess, 320, 0},
    {LCD_EVENT, lcdProcess, 0, 0},
    };


void uartsimProcess() {
    uartsimSend('a');
}

void lcdProcess() {
    sendSomething("lcd ", 4);
    lcdInit(&lcd, 6, 7, 15, 14, 13, 9);
    delay(10);
//    const char *txt = "Labas kaip einasi?";
//    lcdWriteText(&lcd, txt, strlen(txt));
    
//    lcdWriteText(&lcd,(uint8_t[]){ 0b10010000, 0b00101101, ' ', 'e'}, 4);

}

void dallasProc() {
    if (tempstatus == 0) {
        sendSomething("lcdin ", 6);
        lcdProcess();
        tempstatus = 1;
    } else if (tempstatus == 1) {
        sendSomething("stchr ", 6);
        storeChars();
        tempstatus = 2;
    } else  if (tempstatus == 2) {
        sendSomething("tmcfg ", 6);
        wire1Config(&wire1);
        tempstatus = 9; //should be 3
        lcdWriteText(&lcd, "Init", 4);
    } else if (tempstatus == 3) {
        sendSomething("measr ", 6);
        wire1MeasureTemp(&wire1);
        tempstatus = 4;
    } else if (tempstatus == 4) {
        sendSomething("readt ", 6);
        readTemp();
        tempstatus = 3;
    }
}

void measureVoltage() {
    sendSomething("volt  ", 6);
}

void ledBlink() {
    if (ledpos++ %2 == 0) gpioOn(&GPIOB, 5);
    else gpioOff(&GPIOB, 5);
}

void ledBlink2() {
    if (ledpos2++ %2 == 0) gpioOn(&GPIOA, BLINKPIN2);
    else gpioOff(&GPIOA, BLINKPIN2);
}

void ledBlink3() {
    if (ledpos3++ %2 == 0) gpioOn(&GPIOB, BLINKPIN3);
    else gpioOff(&GPIOB, BLINKPIN3);
}

void processEvents() {
    for (int i = 0; i < ALEN(tasks); i++) {
        Task *t = &tasks[i];
        if ((t->period > 0) && (ticks - t->lastTick >= t->period)) {
            t->lastTick = ticks;
            events |= 1 << t->event;
        }

        //if tick counter overflows. Should happen every ~50 days
        if (ticks < t->lastTick) {
            t->lastTick = 0;
        }
    }
}

void TIM2_IRQHandler() {
    ticks++;
    processEvents();
    timerClearInt();
}

void USART2_IRQHandler() {
    receivedData = true;
    uart2Counter++;
    uartDisableInt();
}

void USART3_IRQHandler() {
    receivedSimData = true;
    uartsimDisableInt();
    //timerDisableInt();
    uart3Counter++;
}

void EXTI15_10_IRQHandler() {
    char text[32] = {0};
    sprintf(text, "\r\nPin changed %i\r\n", ++extCounter);
    sendSomething(text, strlen(text));
    motionClearInt();
}


void DMA1_Channel6_IRQHandler() {
    clearDmaIntFlag();
    dmaIntCounter++;
    //eitoa(buffer, dmaIntCounter);

    memcpy(buffer + sizeof(buffer) - 7, rxbuffer, sizeof(rxbuffer));
    sprintf((char *)buffer, "[%li] ", dmaIntCounter);
    const uint8_t mydata[] = {0b10000000};
//    lcdWriteData(&lcd, mydata, sizeof(mydata), 0);
    delaymu(40);
//    lcdWriteText(&lcd, rxbuffer, sizeof(rxbuffer));

}

void sendSomething(const char *lbuf, int len) {
    uint32_t *pSR = (uint32_t*)UART_SR;
    for (int i = 0; i < len; i++) {
        uartSend(lbuf[i]);
        while (!(*pSR &(1 << 7))) { (void)0;};
    }
}

void setup() {
  gpioEnableClock(&GPIOA);
  gpioEnableClock(&GPIOB);
  gpioEnableClock(&GPIOD);
  gpioEnable(&GPIOB, 5, GPIO_OUT);
  gpioEnable(&GPIOA, BLINKPIN2, GPIO_OUT);
  gpioEnable(&GPIOB, BLINKPIN3, GPIO_OUT);
  uartInit();
  uartEnableNVICint();
  uartsimEnableNVICint();
//  initDma();
//  receiveUsartDma(rxbuffer, sizeof(rxbuffer));
  wire1Init(&wire1, &GPIOA, WIRE_PIN);
  timerInit(4, 9000);
  cbufInit(&rxCbuf);
  motionInit(10);
  uartsimInit();
}

void dumpAscii() {
        for (uint8_t i = 0; i < 32; i++) {
            lcdWriteText(&lcd, (uint8_t[]){charPos + i}, 1);
        }

        charPos += 32;
        char buf[13]={0};
        sprintf(buf, "[%i - %i] ", charPos - 32, charPos);
        sendSomething(buf, sizeof(buf));
}

void writeDegrees() {
        const uint8_t buf[] = "Dabar 9";
        lcdWriteText(&lcd, buf, sizeof(buf)-1);
        lcdWriteText(&lcd, (uint8_t[]){(uint8_t)223}, 1);
        lcdWriteText(&lcd, (uint8_t[]){(uint8_t)'C'}, 1);
}

void storeChars() {
    const uint8_t she[] = {
        0b00001010,
        0b00000100,
        0b00001111,
        0b00010000,
        0b00001100,
        0b00000010,
        0b00000001,
        0b00011110
    };

    const uint8_t zhe[] = {
        0b00001010,
        0b00000100,
        0b00011111,
        0b00000001,
        0b00000110,
        0b00001000,
        0b00010000,
        0b00011111
    };

    const uint8_t deg[] = {
        0b00000111,
        0b00000101,
        0b00000111,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000
    };


    lcdWriteRam(&lcd, 0, she);
    delay(1);
    lcdWriteRam(&lcd, 1, zhe);
    delay(1);
    lcdWriteRam(&lcd, 2, deg);
}

void checkTempPresent() {
    if (wire1Reset(&wire1)) {
        sendSomething("present ", 8); 
    } else {
        sendSomething("Not pres ", 9); 
    }
}

void measureTemp() {
    if (wire1MeasureTemp(&wire1) == WIRE1_OK) {
        sendSomething("Meas ok ", 8); 
    } else {
        sendSomething("Meas er ", 8); 
    }
}
void formatTempr(char *buf, uint8_t h, uint8_t l) {
    char *sign = "";
    int16_t combined = (h << 8) | l;
    if (combined < 0) {
        combined = ~combined + 1;
        sign = "-";
        h = ((combined & 0xf00) >> 4) | ((combined & 0xf0) >> 4);
        l = combined & 0x0f;
    } else {
        h = (h << 4) | (l  >> 4);
        l = l & 0x0f;
    }


    int lbig = l * 625;
    int lrem = lbig % 1000;
    int lrnd = lbig / 1000;
    if (lrem >= 500) {
        lrnd += 1;
    }

    sprintf(buf, "%s%i.%01i", sign, h, lrnd);
}

void formatTemprOld(char *buf, uint8_t h, uint8_t l) {
    int lbig = l * 625;
    int lrem = lbig % 1000;
    int lrnd = lbig / 1000;
    if (lrem >= 500) {
        lrnd += 1;
    }

    sprintf(buf, "%i.%01i", h, lrnd);
}

void readTemp() {
    char buf[40] = {0};
    char buft[20] = {0};
    if (wire1ReadTemp(&wire1) == WIRE1_OK) {
        sendSomething("Read ok ", 8);
        //sprintf(buf, "t=%f", wire1.tempr);
        formatTempr(buft, wire1.tmain, wire1.tfrac);
        sprintf(buf, "Tmp1=%s", buft);
        lcdHome(&lcd);
        lcdWriteText(&lcd, buf, strlen(buf));
        lcdWriteText(&lcd, (uint8_t[]){2, 'C'}, 2);
        sprintf(buf, " %i", uart3Counter);
        lcdWriteText(&lcd, buf, strlen(buf));

        sendSomething(buf, sizeof(buf));
        sendSomething(" ", 1);
    } else {
        sendSomething("Read er ", 8); 
    }

    
}

void configTemp() {
    if (wire1Config(&wire1) == WIRE1_OK) {
        sendSomething("Conf ok ", 8); 
    } else {
        sendSomething("Conf er ", 8);
    }
}

void readsimData() {
    uint8_t val = uartsimRead();
    timerDisableInt();
    lcdProcess();
    uint8_t tmpbuf[32];
    sprintf(tmpbuf, "U2:%i, U3:%i", uart2Counter, uart3Counter);
    lcdWriteText(&lcd, tmpbuf, strlen(tmpbuf));
    timerEnableInt();
}

void commandPassThrough() {
    passThrough = !passThrough;
}

void readRxData() {

    uint32_t *pDR = (uint32_t *)UART_DR;
    const uint32_t val = *pDR;
    /*
    if (val == 8) {
        events |= 1 << LCD_EVENT;
    } else {
        dumpAscii();
    }
    return;
    */


   timerDisableInt();


    switch(val) {

        case 8:
            events |= 1 << LCD_EVENT;
            break;
        case 13:
            uint8_t tmpBuf[CIRC_BUF_SIZE];
            int n = cbufRead(&rxCbuf, tmpBuf, sizeof tmpBuf);
            tmpBuf[n] = '\0';
            int rez = commandExec(tmpBuf);
            if (rez) {
                if (!passThrough) {
                    lcdWriteText(&lcd, tmpBuf, strlen(tmpBuf));
                } else {
                    uartsimSendBuf((uint8_t[]){13, 10}, 2);
                }
            }
            break;
        default:
            if (passThrough) {
                uartsimSend(val);
            }
            cbufWrite(&rxCbuf, (uint8_t *)&val, 1);

    }
/*        case 's':
            storeChars();
            sendSomething("Stored ", 7);
            break;
        case 'S':
            lcdWriteText(&lcd, (uint8_t[]){0}, 1);
            break;
        case 'Z':
            lcdWriteText(&lcd, (uint8_t[]){1}, 1);
            break;
        case 'p':
            checkTempPresent();
            break;
        case 'm':
            measureTemp();
            break;
        case 'r':
            readTemp();
            break;
        case 'c':
            configTemp();
            break;
        default:
            lcdWriteText(&lcd, (uint8_t[]){val}, 1);
        }
            */
    timerEnableInt();
}

void checkEvents() {
    for (int i = 0; i < ALEN(tasks); i++) {
        Task *t = &tasks[i];
        if (events & (1 << t->event)) {
            t->func();
            events &= ~(1 << t->event);
        }
    }
}

static bool regsSent = false;
void loop() {
    delay(1);
    if (receivedData) {
        readRxData();
        uartEnableInt();
        receivedData = false;
    }

    if (receivedSimData) {
        readsimData();
       // timerDisableInt();
        uartsimEnableInt();
        receivedSimData = false;
    }

  checkEvents();
  /*
  if (!regsSent) {
       sendSomething("\r\n", 2);
      sendSomething(buffer, sizeof(buffer) -1);
      regsSent = true;
      timerDisableInt();
      sendSomething("\r\n", 2);
  }
  */
}
void fillBufferWithRegs() {
    int regdelta = 0;
        for (int i = 0; i < 9; i++) {
                //rcc
               //uint32_t *p = (uint32_t*)( 0x40021000+ regdelta);
                //gpioD
                //uint32_t *p = (uint32_t*)( 0x40011400+ regdelta);
                //AFIO
                uint32_t *p = (uint32_t*)( 0x40010000 + regdelta);
                sprintf((char *)buffer + (i + 1) * 15, "%x:%lx,", regdelta, *p);
                regdelta+=4;
        }
}

int main(void) {
  clockConfig();
  delay(10);
  setup();
  uartEnableInt();
  uartsimRead();
  uartsimEnableInt();
  timerEnableInt();
  timerStart();
//  fillBufferWithRegs();
  for(;;) {
    loop();
  }
}
