#pragma once
#include <stdint.h>
typedef enum {MODEM_OK = 0, MODEM_LOCKED} MODEM_STATUS;
typedef void (*ModemParserFunc)(void);
void modemProcessResponse();
MODEM_STATUS modemLock(ModemParserFunc func);
MODEM_STATUS modemUnlock(ModemParserFunc func);
void modemAddByte(uint8_t b);
void modemReset();
char *modemGetPart(int index);
