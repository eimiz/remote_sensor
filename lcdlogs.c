#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "lcdlogs.h"
#include "uart.h"
#define MIN_REFRESH_INTERVAL 1000
static char lcdLogs[LLOG_LAST][17] = { 0 };
static int currentPage = 0;
static TLcd *lcd;
static uint32_t lastRefresh = 0;
extern uint32_t ticks;
typedef const char * (*RowFormatter)(void);
static const char * formatter0();
static const char * formatter1();
static const char * formatter2();
static const char * formatter3();
static const char * formatter4();
char formatterBuf[17];
RowFormatter rowFormatters[] = {formatter0, formatter1, formatter2, formatter3, formatter4};

void lcdlogsSet(LcdLogKey key, const char *log) {
    strncpy(lcdLogs[key], log, 16);
    if ((ticks - lastRefresh > MIN_REFRESH_INTERVAL) &&
        (key==currentPage || key == currentPage + 1)) {
        lcdlogsRefresh();
        lastRefresh = ticks;
    }
}

static const char * formatter0() {
    strcpy(formatterBuf, lcdLogs[LLOG_STATUS]);
    return formatterBuf;
}

static const char * formatter1() {
    strcpy(formatterBuf, lcdLogs[LLOG_TMPR]);

    strcat(formatterBuf, " ");
    strncat(formatterBuf, lcdLogs[LLOG_SERVICE_PROVIDER], 4);

    int qval = atoi(lcdLogs[LLOG_LINK_QUALITY]);
    char buf[16];
    sprintf(buf, "%i%%", qval * 100 / 31);
    if (strlen(buf) < 4) {
        strcat(formatterBuf, " ");
    }

    strcat(formatterBuf, (char[]){LCD_ANTENNA_CHR, 0});
    strcat(formatterBuf, buf);
    
    
    return formatterBuf;
}

static const char * formatter2() {
    return formatterBuf;
}

static const char * formatter3() {
    return formatterBuf;
}

static const char * formatter4() {
    return formatterBuf;
}

void lcdlogsRefresh() {
/*    uartSendLog("writing to lcd:[");
    uartSendLog(lcdLogs[currentPage]);
    uartSendLog("]");
    */
    lcdWriteFirstRow(lcd, rowFormatters[currentPage]());
    lcdWriteSecondRow(lcd, rowFormatters[currentPage + 1]());
}

void lcdlogsInit(TLcd *plcd) {
    lcd = plcd;
}


