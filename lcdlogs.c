#include <string.h>
#include "lcdlogs.h"
#include "uart.h"
static char lcdLogs[LLOG_LAST][17] = { 0 };
static int currentPage = 0;
static TLcd *lcd;
void lcdlogsSet(LcdLogKey key, const char *log) {
    strncpy(lcdLogs[key], log, 16);
    if (key==currentPage || key == currentPage + 1) {
        lcdlogsRefresh();
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
