#include <string.h>
#include "lcdlogs.h"
#include "uart.h"
#define MIN_REFRESH_INTERVAL 1000
static char lcdLogs[LLOG_LAST][17] = { 0 };
static int currentPage = 0;
static TLcd *lcd;
static uint32_t lastRefresh = 0;
extern uint32_t ticks;
void lcdlogsSet(LcdLogKey key, const char *log) {
    strncpy(lcdLogs[key], log, 16);
    if ((ticks - lastRefresh > MIN_REFRESH_INTERVAL) &&
        (key==currentPage || key == currentPage + 1)) {
        lcdlogsRefresh();
        lastRefresh = ticks;
    }
}

void lcdlogsRefresh() {
/*    uartSendLog("writing to lcd:[");
    uartSendLog(lcdLogs[currentPage]);
    uartSendLog("]");
    */
    lcdWriteFirstRow(lcd, lcdLogs[currentPage]);
    lcdWriteSecondRow(lcd, lcdLogs[currentPage + 1]);
}

void lcdlogsInit(TLcd *plcd) {
    lcd = plcd;
}
