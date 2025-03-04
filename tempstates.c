#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "circbuf.h"
#include "uart.h"
#include "uartsim.h"
#include "tempstates.h"

#define ASIZE(x) (sizeof(x) / sizeof(x[0]))
#define MIN(x,y) ((x) < (y)) ? (x) : (y)
#define MAX(x,y) ((x) > (y)) ? (x) : (y)

typedef enum {STATE_INIT, STATE_GPRS_INIT, STATE_CONNECTING, STATE_READY} ClState;
typedef struct {
    const char *command;
    const char *submitCommand;
} SimCommand;
const char * const ENDL = "\n";
const char CTRL_Z[] = {26, 0};
const SimCommand  TEST_COMMAND = {"AT", ENDL};
const SimCommand  IDENT_COMMAND = {"ATI", ENDL};
const SimCommand  FAKE_COMMAND = {"ATFFF", ENDL};
const SimCommand  GPRS_ATTACH = {"AT+CGATT=1", ENDL};
const SimCommand  WIRELESS_UP = {"AT+CIICR", ENDL};
const SimCommand  WIRELESS_APN = {"AT+CSTT=\"ezys\"", ENDL};
const SimCommand  IPADDR_COMMAND = {"AT+CIFSR", ENDL}; //returns only ip address, no OK. Mandatory, otherwise ciat+cipstart won't work
const SimCommand  CONN_COMMAND = {"at+cipstart=\"TCP\",\"88.223.53.82\",\"44556\"", ENDL};
const SimCommand  SEND_COMMAND = {"at+cipsend", ENDL};
const SimCommand  TEXT_COMMAND = {"Info from sim800\r\n", CTRL_Z};
const SimCommand  TEXT_COMMAND2 = {"Another Info from sim800!!\r\n", CTRL_Z};

static TBuf cbuf;
const SimCommand * const  ALL_COMMANDS[] = {&TEST_COMMAND, &WIRELESS_APN, &WIRELESS_UP, &IPADDR_COMMAND, &CONN_COMMAND, &SEND_COMMAND, &TEXT_COMMAND, &SEND_COMMAND, &TEXT_COMMAND2};
int currentState = 0;
uint8_t responseBuffer[200];

void tsParseResponse() {
    uartSendStr("Parsing response\r\n");

    uint8_t buf[200];
    int numread = cbufRead(&cbuf, buf, sizeof(buf));

    buf[numread] = '\0';


    if (numread <= 0) {
        strcpy(buf, "wrong response");
    }

    int index = numread - 1;
    while (index >= 0) {
        if (buf[index] == '\r') {
            break;
        }

        index --;
    }

    int bcounter = 0;
    while (index >0) {
        if (buf[--index] == '\n') {
            break;
        }

        responseBuffer[bcounter++] = buf[index];
    }

    //reverse:
    for (int i = 0; i < bcounter / 2; i++) {
        uint8_t tmp = responseBuffer[i];
        responseBuffer[i] = responseBuffer[bcounter -i - 1];
        responseBuffer[bcounter -i - 1] = tmp;
    }

    //end string
    responseBuffer[bcounter] = '\0';
}

void tsProcessResponse() {
    tsParseResponse();
    if (strcmp(responseBuffer, "ERROR") != 0) {
        currentState = MIN(ASIZE(ALL_COMMANDS), currentState + 1);
    } else {
        currentState = MAX(0, currentState);
    }

    uartSendStr("Current state is ");
    uartSend('0' + currentState);
    uartSendStr("\r\n");
    if (currentState < ASIZE(ALL_COMMANDS)) {
        tsRunState();
    }
}

void uartWriteFunc(const SimCommand *c) {
    uartsimSendBuf(c->command, strlen(c->command));
    uartsimSendBuf(c->submitCommand, strlen(c->submitCommand));
}


void tsInitTempStates() {
    cbufInit(&cbuf);
    currentState = 0;
}

void execCommand (const SimCommand *c) {
    uartSendStr("***execing command ");
    uartSendStr(c->command);
    uartSendStr("\r\n");
    uartWriteFunc(c);
}

void tsRunState() {
    if (currentState < ASIZE(ALL_COMMANDS)) {
        execCommand(ALL_COMMANDS[currentState]);
    }
}

void tsAddByte(uint8_t b) {
    cbufWrite(&cbuf, &b, 1);
}
