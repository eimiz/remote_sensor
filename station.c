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
#include "lcdlogs.h"
#include "modem.h"
#include "tokenize.h"
#include "button.h"


#define RESETPIN 6
#define BLINKPIN3 12
uint8_t buffer[200] = {"Pradzia! "};
uint8_t rxbuffer[3] = {"***"};
//int annCnt = 0;
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

void ledBlink3(void *t);
void autostartProcess(void *t);
void simrxWatch(void *t);
void uartsimProcess(void *t);
static void linkQualityProcess(void *t);
static void serviceProviderProcess(void *t);


uint32_t ticks = 0;
int tempstatus = 0;

Task simrxWatchTask = {SIMRX_WATCH_EVENT, simrxWatch, 0, 0, false};
//Task blink1Task = {BLINK_EVENT, ledBlink, 500, 0, true};
Task serviceProviderTask = {SERVICE_PROVIDER_EVENT, serviceProviderProcess, 15000, 0, true};
//Task dallasTask = {TEMPR_EVENT, dallasProc, 1500, 0, true};

Task tasks[] = {
//    {MOTION_EVENT, measureVoltage, 6300, 0},

    {BLINK3_EVENT, ledBlink3, 320, 0, true},
    {TEMPR_EVENT, dallasProc, 1500, 0, true},
    {SIMPROCESS_EVENT, uartsimProcess, 0, 0, false},

    {LINK_QUALITY_EVENT, linkQualityProcess, 10000, 0, true},
//    {AUTOSTART_EVENT, autostartProcess, 20000, 0, true},
};

void readTemp();
int runningTasksCount = 0;
Task *runningTasks[32];

void stationPostponeTask(Task *task, uint32_t period) {
    task->lastTick = ticks;
    task->period = period;
    task->active = true;
}

static void serviceProviderParser() {
    uartSendLog("Service provider parser");
    int pcnt = modemPartsCount();
    char buf[128];
    
    if (pcnt < 3) {
        sprintf(buf, "srv parts count: %i less than 3", pcnt);
        uartSendLog(buf);
        modemUnlock(serviceProviderParser);
        return;
    }

    const char *srvstr = modemGetPart(1);
    sprintf(buf, "csqstr is [%s]", srvstr);
    uartSendLog(buf);
    char *parts[TOKENIZE_MAX_PARTS];
    int partsCount = tokenize2(parts, srvstr, "\"");
    if (partsCount != 3) {
        sprintf(buf, "srv provider parts count: %i not equal to 3", partsCount);
        modemUnlock(serviceProviderParser);
        return;
    }

    lcdlogsSet(LLOG_SERVICE_PROVIDER, parts[1]);
    sprintf(buf, "service provider is [%s]\n", parts[1]);
    uartSendLog(buf);
    modemUnlock(serviceProviderParser);
    stationStopTask(&serviceProviderTask);
}

static void linkQualityParser() {
    uartSendLog("quality parser");
    int pcnt = modemPartsCount();
    char buf[128];
    
    if (pcnt < 3) {
        sprintf(buf, "parts count: %i less than 3", pcnt);
        uartSendLog(buf);
        modemUnlock(linkQualityParser);
        return;
    }

    const char *csqstr = modemGetPart(1);
    sprintf(buf, "csqstr is [%s]", csqstr);
    uartSendLog(buf);
    char *parts[TOKENIZE_MAX_PARTS];
    int csqPartsCount = tokenize2(parts, csqstr, " ,");
    if (csqPartsCount != 3) {
        sprintf(buf, "csq parts count: %i not equal to 3", csqPartsCount);
        modemUnlock(linkQualityParser);
        return;
    }

    lcdlogsSet(LLOG_LINK_QUALITY, parts[1]);
    sprintf(buf, "csq quality is [%s]\n", parts[1]);
    uartSendLog(buf);
    modemUnlock(linkQualityParser);
}

static void serviceProviderProcess(void *t) {
    uartSendLog("Service provider process");
    if (modemLock(serviceProviderParser) != MODEM_OK) {
        return;
    }

    uartsimSendStr("at+cspn?\n");

}

static void linkQualityProcess(void *t) {
    uartSendLog("quality process");
    if (modemLock(linkQualityParser) != MODEM_OK) {
        return;
    }

    uartsimSendStr("at+csq\n");
    stationStopTask((Task *)t);
}


void autostartProcess(void *t) {
    commandExec("states");
    stationStopTask((Task *)t);
}


void stationResetModem() {
    gpioOff(&GPIOA, RESETPIN);
    delay(2000);
    gpioOn(&GPIOA, RESETPIN);
}

bool isTaskRegistered(Task *task) {
	for (int i = 0; i < runningTasksCount; i++) {
        if (runningTasks[i] == task) {
            return true;
        }
	}

    return false;
}

bool stationIsTaskRunning(Task *task) {
	for (int i = 0; i < runningTasksCount; i++) {
        if (runningTasks[i] == task) {
            return task->active;
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
/*
	if (isTaskRegistered(&dallasTask) && dallasTask.active) {
        stationStopTask(&dallasTask);
    } else  {
        stationStartTask(&dallasTask);
    } 
*/
}
void stationRegisterEvent(TEvent event) {
    events |= 1 << event;
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
   modemProcessResponse();
}

void lcdSetup() {
    uartSendStr("lcd ");
    lcdInit(&lcd, 6, 7, 15, 14, 13, 9);
    delay(10);
    lcdStoreChars(&lcd);
    delay(1);
//    const char *txt = "Labas kaip einasi?";
//    lcdWriteText(&lcd, txt, strlen(txt));
    
//    lcdWriteText(&lcd,(uint8_t[]){ 0b10010000, 0b00101101, ' ', 'e'}, 4);

}

void dallasProc(void *p) {
/*    sprintf(buf, "This is annoying text %i", annCnt++);
    uartSendLog(buf);
    return;
    */
    if (tempstatus == 0) {
        uartSendStr("tmcfg ");
        wire1Config(&wire1);
        tempstatus = 1;
        lcdWriteText(&lcd, "Init", 4);
    } else if (tempstatus == 1) {
//        uartSendStr("measr ");
        wire1MeasureTemp(&wire1);
        tempstatus = 2;
    } else if (tempstatus == 2) {
//        uartSendStr("readt ");
        readTemp();
        tempstatus = 1;
    }
}

void measureVoltage(void *p) {
    uartSendStr("volt  ");
}

void stationLedToggle(void *p) {
    if (ledpos++ %2 == 0) gpioOn(&GPIOB, 5);
    else gpioOff(&GPIOB, 5);

lcdlogsNext();
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
    buttonProbe();
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
    uint8_t val = uartsimRead();
    modemAddByte(val);
    simDataReceived = true;
    simWatchByteReceived = true;
   // uartsimDisableInt();
    uart3Counter++;
    if (UART3->SR & (1 << 3)) {
        oreCounter++;
    }

   // uartsimEnableInt();
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
  gpioEnable(&GPIOA, RESETPIN, GPIO_OUT);
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
  gpioOn(&GPIOA, RESETPIN);
  lcdSetup();
  lcdlogsInit(&lcd);
  buttonInit();
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

void readTemp() {
    char buf[40] = {0};
    char buft[20] = {0};
    if (wire1ReadTemp(&wire1) == WIRE1_OK) {
//        uartSendStr("Read ok ");
        //sprintf(buf, "t=%f", wire1.tempr);
        eutilsFormatTempr(buft, wire1.tmain, wire1.tfrac);
//        sprintf(buf, "Tmp1=%s", buft);
//        uartSendLog(buf);
        strcat(buft, (char[]){2, 'C', 0});
        lcdlogsSet(LLOG_TMPR, buft);

/*
        sprintf(buf, " %i", uart3Counter);
        lcdWriteText(&lcd, buf, strlen(buf));

        uartSendStr(buf);
        uartSendStr(" ");
*/
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

   // uartSendStr("**r");
	if (!passThrough) {
	    stationPostponeTask(&simrxWatchTask, 1200);
	}
}

void stationReportUartStats() {
    char buf[128];
    sprintf(buf, "sim overr: %i, txcnt: %i u2cnt: %i dma: %i", oreCounter, uart3Counter, uart2Counter, dmaIntCounter);
    uartSendLog(buf);
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

/*        case 8:
//            events |= 1 << LCD_EVENT;
            lcdSetup();
            break;
            */
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
       // uartsimEnableInt();
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

  stationRegisterTask(&serviceProviderTask);
//  fillBufferWithRegs();
  for(;;) {
    loop();
  }
}


