#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "eutils.h"
#include "uartsim.h"
#include "uart.h"


#include "tempstates.h"
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
void commandStartDallas();
void stationStartDallas();
void stationGetOreCounter();
TCommand commands[] = {{"hello", commandHello}, {"another", commandAnother}, 
{"pass", commandPassThrough}, {"states", commandRunState}, {"dallas", commandStartDallas}, {"ore",
stationGetOreCounter }};

void commandStartDallas() {
    stationStartDallas();
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
    tsInitTempStates();
    tsRunState();
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

