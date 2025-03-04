#include <string.h>
#include "eutils.h"
#include "uartsim.h"
#include "uart.h"

#include "tempstates.h"

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
TCommand commands[] = {{"hello", commandHello}, {"another", commandAnother}, 
{"pass", commandPassThrough}, {"states", commandRunState}, {"dallas", commandStartDallas}};

void commandStartDallas() {
    stationStartDallas();
}

void commandHello() {
    uartSendStr("Hello received\r\n");
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

