#include <stdbool.h>
#include <string.h>
#include "circbuf.h"
#include "uart.h"
#include "tokenize.h"
#include "modem.h"
#include "station.h"
#include "uartsim.h"
static TBuf cbuf;
static void defaultParser();
static ModemParserFunc currentParser = defaultParser;
static bool locked = false;
static char bigbuf[200];
static int partsCount = 0;
static bool simWatchByteReceived = false;
static void simrxWatch(void *t);
static void uartsimProcess(void *t);
static void simPostponeWatch(void *t);
static Task simrxWatchTask = {SIMRX_WATCH_EVENT, simrxWatch, 0, 0, false};
static Task simPostponeWatchTask = {SIM_POSTPONE_WATCH_EVENT, simPostponeWatch, 0, 0, false};
static Task simProcessTask = {SIMPROCESS_EVENT, uartsimProcess, 0, 0, false};

int oreCounter = 0;
int uart3Counter = 0;
char *modemResponseParts[TOKENIZE_MAX_PARTS];

char *modemGetPart(int index) {
    return modemResponseParts[index];
}

int modemPartsCount() {
    return partsCount;
}

MODEM_STATUS modemLock(ModemParserFunc func) {
    if (locked) {
        uartSendLog("Modem already locked");
        return MODEM_LOCKED;
    }

    locked = true;
    currentParser = func;
    uartSendLog("Locking modem");
    return MODEM_OK;
}

MODEM_STATUS modemUnlock(ModemParserFunc func) {
    if (!locked) {
        uartSendLog("Unloking free modem, strange");
        return MODEM_OK;
    }

    if (func != currentParser) {
        uartSendLog("Cannot unlock not your modem");
        return MODEM_LOCKED;
    }

    locked = false;
    currentParser = defaultParser;
    uartSendLog("Unlocking modem");
    return MODEM_OK;
}

static void defaultParser() {
    uartSendLog("defaul processor invoked");
}

void modemAddByte(uint8_t b) {
    cbufWrite(&cbuf, &b, 1);
}

static void simPostponeWatch(void *t) {
	if (!stationIsPassThrough()) {
	    stationPostponeTask(&simrxWatchTask, 1200);
	}
}

static void modemParseResponse() {
    uartSendStr("Parsing response\r\n");

    int numread = cbufRead(&cbuf, bigbuf, sizeof(bigbuf));

    bigbuf[numread] = '\0';
    uartSendLog("Raw buf:");
    uartSendLog(bigbuf);

    if (numread <= 0) {
        strcpy(bigbuf, "wrong response");
    }

    partsCount = tokenize(modemResponseParts, bigbuf);
}

void modemProcessResponse() {
    modemParseResponse();
    currentParser();
}

void modemReset() {
    cbufInit(&cbuf);
}

static void simrxWatch(void *pt) {
//    return;
    Task *t = (Task *)pt;
    uartSendLog("[watch]");
    if (simWatchByteReceived) {
        uartSendLog("[not yet]");
        simWatchByteReceived = false;
    } else {
        uartSendLog("[[simdataNotReceived]]");
        stationRegisterEvent(SIMPROCESS_EVENT);
        //stop it
        t->active = false;
    }
}

static void uartsimProcess(void *p) {
   uartSendLog("[xrocess]");
   modemProcessResponse();
}


void USART3_IRQHandler() {
    uint8_t val = uartsimRead();
    modemAddByte(val);
    simWatchByteReceived = true;
    stationRegisterEvent(SIM_POSTPONE_WATCH_EVENT);
    uart3Counter++;
    if (UART3->SR & (1 << 3)) {
        oreCounter++;
    }

}

void modemInit() {
  stationRegisterTask(&simrxWatchTask);
  stationRegisterTask(&simPostponeWatchTask);
  stationRegisterTask(&simProcessTask);
}
