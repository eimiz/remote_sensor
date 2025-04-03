#pragma once
#include <stdbool.h>
#include "circbuf.h"
void tsRunState();
void tsInitTempStates();
void tsProcessResponse();
void tsAddByte(uint8_t b);
void tsStart();
void tsStop();
bool tsIsRunning();
void tsResetModemRestartStates();
