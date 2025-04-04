#include <stdbool.h>
#include <string.h>
#include "circbuf.h"
#include "uart.h"
#include "tokenize.h"
#include "modem.h"
static TBuf cbuf;
static void defaultParser();
static ModemParserFunc currentParser = defaultParser;
static bool locked = false;
static char bigbuf[200];
static int partsCount = 0;
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

