#pragma once
#include "lcd.h"
typedef enum {LLOG_STATUS = 0, LLOG_TMPR, LLOG_SERVICE_PROVIDER, LLOG_LINK_QUALITY,
LLOG_PACKETS_SENT, LLOG_RESTART_COUNT, LLOG_LAST} LcdLogKey;
void lcdlogsSet(LcdLogKey key, const char *log);
void lcdlogsRefresh();
void lcdlogsInit(TLcd *plcd);
void lcdlogsNext();
