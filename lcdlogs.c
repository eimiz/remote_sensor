#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "lcdlogs.h"
#include "uart.h"
#include "motion.h"
#include "modem.h"
#include "tokenize.h"
#include "uartsim.h"
#include "station.h"
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
static const char * formatter5();
static const char * formatter6();
char formatterBuf[17];
static void timeRefresh(void *t);
RowFormatter rowFormatters[] = {formatter0, formatter1, formatter2, formatter3, formatter4, formatter5, formatter6};

#define MAX_PAGES (sizeof(rowFormatters) / sizeof(rowFormatters[0]) - 1)
Task timeRefreshTask = {TIME_REFRESH_EVENT, timeRefresh, 1600, 0, true};

void lcdlogsSet(LcdLogKey key, const char *log) {
    strncpy(lcdLogs[key], log, 16);
    char tmpbuf[32];
    if ((ticks - lastRefresh > MIN_REFRESH_INTERVAL)
        // &&    (key==currentPage || key == currentPage + 1))
        )
        {
        lcdlogsRefresh();
        lastRefresh = ticks;
        sprintf(tmpbuf, "rticks:%i", lastRefresh);
        uartSendLog(tmpbuf);
        
    } else {

        sprintf(tmpbuf, "curp: %i, key:%i", currentPage, key);
        uartSendLog("Will not refresh");
        uartSendLog(tmpbuf);
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
    strcpy(formatterBuf, "Service: ");
    strncat(formatterBuf, lcdLogs[LLOG_SERVICE_PROVIDER], 7);
    return formatterBuf;
}

static const char * formatter3() {
    strcpy(formatterBuf, "Link q: ");
    strncat(formatterBuf, lcdLogs[LLOG_LINK_QUALITY], 4);
    return formatterBuf;
}

static const char * formatter4() {
    strcpy(formatterBuf, "Judejimas: ");
    if (motionPresent())
        strcat(formatterBuf, "TAIP");
    else
        strcat(formatterBuf, "NE");

    return formatterBuf;
}

static void timeParser() {
    if (currentPage != 4 &&  currentPage != 5) {
        modemUnlock(timeParser);
        stationStopTask(&timeRefreshTask);
    }

   uartSendLog("Parsing time"); 
    int pcnt = modemPartsCount();
    char buf[128];
    
    if (pcnt < 3) {
        sprintf(buf, "time parser, parts count: %i less than 3", pcnt);
        uartSendLog(buf);
        modemUnlock(timeParser);
        return;
    }
    const char *timestr = modemGetPart(1);
    char *parts[TOKENIZE_MAX_PARTS];
    int timePartsCount = tokenize2(parts, timestr, ",+\"");
    if (timePartsCount != 4) {
        sprintf(buf, "time parser, tm parts count: %i not eq to 4", timePartsCount);
        modemUnlock(timeParser);
        return;

    }
    char *dateparts[TOKENIZE_MAX_PARTS];
    int datepartsCount = tokenize2(dateparts, parts[1], "/");

    if (datepartsCount != 3) {
        sprintf(buf,"Date parts count: %i not equal to 3", datepartsCount);
        modemUnlock(timeParser);
        return;
    }
    
    sprintf(buf, "20%s-%s-%s", dateparts[0], dateparts[1], dateparts[2]);
    lcdlogsSet(LLOG_TIME, parts[2]);
    lcdlogsSet(LLOG_DATE, buf);
    lcdlogsRefresh();
    uartSendLog("Got date, time");
    uartSendLog(buf);
    uartSendLog(parts[2]);
    modemUnlock(timeParser);



}

static const char * formatter5() {
    uartSendLog("Drawing time");
    strcpy(formatterBuf, "Laiks: ");
    strcat(formatterBuf, lcdLogs[LLOG_TIME]);
    if (!stationIsTaskRunning(&timeRefreshTask)) {
        stationStartTask(&timeRefreshTask);
    }

    return formatterBuf;
}

static const char * formatter6() {
    uartSendLog("Drawing date");
    strcpy(formatterBuf, "Data: ");
    strcat(formatterBuf, lcdLogs[LLOG_DATE]);
    if (!stationIsTaskRunning(&timeRefreshTask)) {
        stationStartTask(&timeRefreshTask);
    }

    return formatterBuf;
}

static void timeRefresh(void *t) {

    if (modemLockSpeed(timeParser, 200) != MODEM_OK) {
        return;
    }

    uartsimSendStr("at+cclk?\n");
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

void lcdlogsNext() {
    currentPage++;
    if (currentPage == MAX_PAGES) {
        currentPage = 0;
    }

    lcdlogsRefresh();
}
