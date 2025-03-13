#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "station.h"
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
#include "tempstates.h"
#include "tempstream.h"

#define WIRE_PIN 0
#define BLINKPIN2 6
#define BLINKPIN3 12
uint8_t buffer[200] = {"Pradzia! "};
uint8_t rxbuffer[3] = {"***"};

uint16_t bpos = 0;
static bool receivedData = false;
static bool simDataReceived = false;
static bool simWatchByteReceived = false;
int32_t dmaIntCounter = 0;
static bool passThrough = false;
int ledpos = 1;
int ledpos2 = 1;
int ledpos3 = 1;
uint8_t charPos = 0;
static int extCounter = 0;
int uart3Counter = 0;
int uart2Counter = 0;
int intEnaCounter = 0;
int intDisCounter = 0;
int oreCounter = 0;
TLcd lcd;
static TWire1 wire1;
TBuf rxCbuf;
char buftmp[20];
uint32_t events;
void dallasProc(void *t);
void measureVoltage(void *t);
void ledBlink(void *t);
void ledBlink2(void *t);
void ledBlink3(void *t);
void lcdProcess(void *t);
void simrxWatch(void *t);
void uartsimProcess(void *t);


void storeChars();



uint32_t ticks = 0;
int tempstatus = 0;

Task simrxWatchTask = {SIMRX_WATCH_EVENT, simrxWatch, 0, 0, false};
Task blink1Task = {BLINK_EVENT, ledBlink, 500, 0, true};
Task dallasTask = {TEMPR_EVENT, dallasProc, 1500, 0, true};

Task tasks[] = {
//    {TEMPR_EVENT, dallasProc, 1500, 0},
//    {MOTION_EVENT, measureVoltage, 6300, 0},

    {BLINK2_EVENT, ledBlink2, 284, 0, true},
    {BLINK3_EVENT, ledBlink3, 320, 0, true},
    {SIMPROCESS_EVENT, uartsimProcess, 0, 0, false},
    };

void readTemp();
int runningTasksCount = 0;
Task *runningTasks[32];

void postponeTask(Task *task, uint32_t period) {
    task->lastTick = ticks;
    task->period = period;
    task->active = true;
}

bool isTaskRegistered(Task *task) {
	for (int i = 0; i < runningTasksCount; i++) {
        if (runningTasks[i] == task) {
            return true;
        }
	}

    return false;
}

void stationRegisterTask(Task *task) {
    if (!isTaskRegistered(task)) {
        runningTasks[runningTasksCount++] = task;
    }
}

void stationStartTask(Task *task) {
    if (!isTaskRegistered(task)) {
        runningTasks[runningTasksCount++] = task;
    }

    task->active = true;
}

void stationStopTask(Task *task) {
    task->active = false;
}

void stationDallas() {
    char buf[32];
    for (int i = 0; i < runningTasksCount; i++) {
        Task *t = runningTasks[i];
        sprintf(buf, "t:%i, act:%i|", i, t->active);
        uartSendLog(buf);
    }

	if (!dallasTask.active) {
        stationStartTask(&dallasTask);
    } else  {
        stationStopTask(&dallasTask);
    } 
}

void simrxWatch(void *pt) {
//    return;
    Task *t = (Task *)pt;
    uartSendStr("\r\n[watch]\r\n");
    if (simWatchByteReceived) {
        uartSendStr("\r\n[not yet]\r\n");
        simWatchByteReceived = false;
    } else {
        uartSendStr("\r\n[[simdataNotReceived]]\r\n");
        events |= 1 << SIMPROCESS_EVENT;
        //stop it
        t->active = false;
        uartSend('s');
    }
}

void uartsimProcess(void *p) {
   uartSendStr("[xrocess]");
   tsProcessResponse();
}

void lcdProcess(void *p) {
    uartSendStr("lcd ");
    lcdInit(&lcd, 6, 7, 15, 14, 13, 9);
    delay(10);
//    const char *txt = "Labas kaip einasi?";
//    lcdWriteText(&lcd, txt, strlen(txt));
    
//    lcdWriteText(&lcd,(uint8_t[]){ 0b10010000, 0b00101101, ' ', 'e'}, 4);

}

void dallasProc(void *p) {
    if (tempstatus == 0) {
        uartSendStr("lcdin ");
        lcdProcess(p);
        tempstatus = 1;
    } else if (tempstatus == 1) {
        uartSendStr("stchr ");
        storeChars();
        tempstatus = 2;
    } else  if (tempstatus == 2) {
        uartSendStr("tmcfg ");
        wire1Config(&wire1);
        tempstatus = 3; //should be 3
        lcdWriteText(&lcd, "Init", 4);
    } else if (tempstatus == 3) {
        uartSendStr("measr ");
        wire1MeasureTemp(&wire1);
        tempstatus = 4;
    } else if (tempstatus == 4) {
        uartSendStr("readt ");
        readTemp();
        tempstatus = 3;
    }
}

void measureVoltage(void *p) {
    uartSendStr("volt  ");
}

void ledBlink(void *p) {
    if (ledpos++ %2 == 0) gpioOn(&GPIOB, 5);
    else gpioOff(&GPIOB, 5);
}

void ledBlink2(void *p) {
    if (ledpos2++ %2 == 0) gpioOn(&GPIOA, BLINKPIN2);
    else gpioOff(&GPIOA, BLINKPIN2);
}

void ledBlink3(void *p) {
    if (ledpos3++ %2 == 0) gpioOn(&GPIOB, BLINKPIN3);
    else gpioOff(&GPIOB, BLINKPIN3);
}

void processTasks() {
    for (int i = 0; i < runningTasksCount; i++) {
        Task *t = runningTasks[i];
        if (t->active && (ticks - t->lastTick >= t->period)) {
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
    processTasks();
    timerClearInt();
}

void USART2_IRQHandler() {
    receivedData = true;
    uart2Counter++;
    uartDisableInt();
    intDisCounter++;
    if (UART2->SR & (1 << 3)) {
     //   oreCounter++;
    }
}

void USART3_IRQHandler() {
    simDataReceived = true;
    simWatchByteReceived = true;
    uartsimDisableInt();
    //timerDisableInt();
    uart3Counter++;
    if (UART3->SR & (1 << 3)) {
        oreCounter++;
    }
}

void EXTI15_10_IRQHandler() {
    char text[32] = {0};
    sprintf(text, "\r\nPin changed %i\r\n", ++extCounter);
    uartSendStr(text);
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
  tsInitTempStates();
}

void dumpAscii() {
        for (uint8_t i = 0; i < 32; i++) {
            lcdWriteText(&lcd, (uint8_t[]){charPos + i}, 1);
        }

        charPos += 32;
        char buf[13]={0};
        sprintf(buf, "[%i - %i] ", charPos - 32, charPos);
        uartSendStr(buf);
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
        uartSendStr("present ");
    } else {
        uartSendStr("Not pres ");
    }
}

void measureTemp() {
    if (wire1MeasureTemp(&wire1) == WIRE1_OK) {
        uartSendStr("Meas ok ");
    } else {
        uartSendStr("Meas er ");
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
        uartSendStr("Read ok ");
        //sprintf(buf, "t=%f", wire1.tempr);
        formatTempr(buft, wire1.tmain, wire1.tfrac);
        sprintf(buf, "Tmp1=%s", buft);
        uartSendStr(buf);
        lcdHome(&lcd);
        lcdWriteText(&lcd, buf, strlen(buf));
        lcdWriteText(&lcd, (uint8_t[]){2, 'C'}, 2);
        sprintf(buf, " %i", uart3Counter);
        lcdWriteText(&lcd, buf, strlen(buf));

        uartSendStr(buf);
        uartSendStr(" ");
    } else {
        uartSendStr("Read er ");
    }
}

void readAndSendTemp() {
    char buf[40] = {0};
    char buft[20] = {0};
    if (wire1ReadTemp(&wire1) == WIRE1_OK) {
        uartSendStr("Read ok ");
        //sprintf(buf, "t=%f", wire1.tempr);
        formatTempr(buft, wire1.tmain, wire1.tfrac);
        sprintf(buf, "Tmp1=%s", buft);
        uartSendStr(buf);
        lcdHome(&lcd);
        lcdWriteText(&lcd, buf, strlen(buf));
        lcdWriteText(&lcd, (uint8_t[]){2, 'C'}, 2);
        sprintf(buf, " %i", uart3Counter);
        lcdWriteText(&lcd, buf, strlen(buf));

        uartSendStr(buf);
        uartSendStr(" ");
    } else {
        uartSendStr("Read er ");
    }
}

void configTemp() {
    if (wire1Config(&wire1) == WIRE1_OK) {
        uartSendStr("Conf ok ");
    } else {
        uartSendStr("Conf er ");
    }
}

void readsimData() {
    uint8_t val = uartsimRead();
    tsAddByte(val);
    uartEnableInt();
   // uartSendStr("**r");
	if (!passThrough) {
	    postponeTask(&simrxWatchTask, 1200);
	}
}

void commandPassThrough() {
    passThrough = !passThrough;
	if (passThrough)
		uartSendStr("\r\nPass through ON\r\n");
	else
		uartSendStr("\r\nPass through OFF\r\n");
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
//            events |= 1 << LCD_EVENT;
            lcdProcess(NULL);
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
            uartSendStr("Stored ", 7);
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
    for (int i = 0; i < runningTasksCount; i++) {
        Task *t = runningTasks[i];
        if (events & (1 << t->event)) {
            t->func(t);
            events &= ~(1 << t->event);
        }
    }
}

static bool regsSent = false;
void loop() {
    //delaymu(1);
//    delay(1);
    if (receivedData) {
        readRxData();

        intEnaCounter++;
        receivedData = false;
        uartEnableInt();
    }

    if (simDataReceived) {
        readsimData();
       // timerDisableInt();
        simDataReceived = false;
        uartsimEnableInt();
    }

  checkEvents();
  /*
  if (!regsSent) {
       uartSendStr("\r\n", 2);
      uartSendStr(buffer, sizeof(buffer) -1);
      regsSent = true;
      timerDisableInt();
      uartSendStr("\r\n", 2);
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

void stationGetOreCounter() {
    char buf[32];
    sprintf(buf, "Ore cnt=%i\r\n", oreCounter);
    uartSendStr(buf);
}


void stationHandshakeFinishedCallback() {
    tempStreamStart();
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
  for (int i = 0; i < ALEN(tasks); i++) {
      stationRegisterTask(&tasks[i]);
  }

  stationRegisterTask(&simrxWatchTask);
  stationRegisterTask(&blink1Task);
//  fillBufferWithRegs();
  for(;;) {
    loop();
  }
}


