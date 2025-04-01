#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "eutils.h"
#include "uartsim.h"
#include "uart.h"
#include "tempstates.h"
#include "tempstream.h"
extern uint32_t ticks;
typedef void (*ComFunc)(void);

typedef struct {
    const char *cmd;
    ComFunc func;
} TCommand;

void commandHello();
void commandAnother();
void commandPassThrough();
void commandRunState();
void commandDallas();
void stationDallas();
void stationGetOreCounter();
TCommand commands[] = {{"hello", commandHello}, {"another", commandAnother}, 
{"pass", commandPassThrough}, {"states", commandRunState}, {"dallas", commandDallas}, {"ore",
stationGetOreCounter }, {"s", commandRunState}};

void commandDallas() {
    stationDallas();
}

void commandHello() {
    char buf[32];
    uartSendStr("Hello received\r\n");
    srand(ticks);
    int r = rand();
    //int r = 0;
    sprintf(buf, "r=%i\r\n", r);
    uartSendStr(buf);
}

void commandAnother() {
    uartSendStr("Another received\r\n");
}

void commandRunState() {
    if (!tsIsRunning()) {
        uartSendLog("Starting states");
        tsInitTempStates();
        tsSetRunning(true);
        tsRunState();
    } else {
        uartSendLog("Stopping states");
        tsSetRunning(false);
        tempStreamStop();
    }
}

int commandExec(const char *cmd) {
    for (int i = 0; i < ALEN(commands); i++) {
        TCommand *c = &commands[i];
        if (strcmp(c->cmd, cmd) == 0) {
            c->func();
            return 0;
        }
    }

    return 1;
}

