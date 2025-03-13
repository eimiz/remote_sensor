#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "circbuf.h"
#include "uart.h"
#include "uartsim.h"
#include "tempstates.h"
#include "eprotocol.h"

#define ASIZE(x) (sizeof(x) / sizeof(x[0]))
#define MIN(x,y) ((x) < (y)) ? (x) : (y)
#define MAX(x,y) ((x) > (y)) ? (x) : (y)

#define NONCE_LEN 12

void stationHandshakeFinishedCallback();
static void commandHelloMagic();
static void commandConsumeSentOk();
static void commandConsumeNonceAndHash();
static void commandSendClientHash();
typedef void (*CommandFunc)();
typedef enum {STATE_INIT, STATE_GPRS_INIT, STATE_CONNECTING, STATE_READY} ClState;
typedef struct {
    CommandFunc comFunc;
    const char *command;
    const char *submitCommand;
} SimCommand;
const char * const ENDL = "\n";
const char CTRL_Z[] = {26, 0};
const SimCommand  TEST_COMMAND = {NULL, "AT", ENDL};
const SimCommand  IDENT_COMMAND = {NULL, "ATI", ENDL};
const SimCommand  FAKE_COMMAND = {NULL, "ATFFF", ENDL};
const SimCommand  GPRS_ATTACH = {NULL, "AT+CGATT=1", ENDL};
const SimCommand  WIRELESS_UP = {NULL, "AT+CIICR", ENDL};
const SimCommand  WIRELESS_APN = {NULL, "AT+CSTT=\"ezys\"", ENDL};
const SimCommand  IPADDR_COMMAND = {NULL, "AT+CIFSR", ENDL}; //returns only ip address, no OK. Mandatory, otherwise ciat+cipstart won't work
const SimCommand  CONN_COMMAND = {NULL, "at+cipstart=\"TCP\",\"88.223.53.82\",\"44556\"", ENDL};
const SimCommand  SEND_COMMAND = {NULL, "at+cipsend", ENDL};
const SimCommand  TEXT_COMMAND = {NULL, "Info from sim800\r\n", CTRL_Z};
const SimCommand  TEXT_COMMAND2 = {NULL, "Another Info from sim800!!\r\n", CTRL_Z};
const SimCommand HELLO_MAGIC = {commandHelloMagic, NULL, CTRL_Z};
const SimCommand CONSUME_SENTOK_CMD = {commandConsumeSentOk, NULL, NULL};
const SimCommand CONSUME_NONCEHASH_CMD = {commandConsumeNonceAndHash, NULL, CTRL_Z};
const SimCommand SEND_CLIENT_HASH_CMD = {commandSendClientHash, NULL, CTRL_Z};


static TBuf cbuf;
//const SimCommand * const  ALL_COMMANDS[] = {&TEST_COMMAND, &WIRELESS_APN, &WIRELESS_UP, &IPADDR_COMMAND, &CONN_COMMAND, &SEND_COMMAND, &TEXT_COMMAND, &SEND_COMMAND, &TEXT_COMMAND2};
const SimCommand * const  ALL_COMMANDS[] = {&TEST_COMMAND, &WIRELESS_APN, &WIRELESS_UP, &IPADDR_COMMAND, &CONN_COMMAND,
 &SEND_COMMAND, &HELLO_MAGIC, &CONSUME_NONCEHASH_CMD,
 &SEND_COMMAND, &SEND_CLIENT_HASH_CMD };
static int currentState = 0;
uint8_t responseBuffer[200];

static void commandConsumeSentOk() {
    //buffer should contain Send OK
    if (strcmp(responseBuffer, "SEND OK")) {
        uartSendLog("got SEND OK");
    } else {
        uartSendLog("Oh no, no SEND OK");
    }

}

static void commandConsumeNonceAndHash() {
    uartSendStr("\r\nProcessing nonce and hash\r\n");
    EproRez rez = eproReadServerNonces(responseBuffer);
    uint8_t buf[32];

    if (rez != EPRO_OK) {
        sprintf(buf, "Error, code=%i", rez);
        uartSendLog(buf);
		return;

    }

    currentState++;
    tsRunState();
}

static void commandSendClientHash() {
    //rand128bitnonce, hash
    uint8_t outbuffer[(CLIENT_HASH_LEN * 8 + 6 - 1)/6];
    uartSendLog("creating client hash");
    eproCreateClientHash(outbuffer);
    uartSendLog("sending client hash");
    uartsimSendBuf(outbuffer, sizeof(outbuffer));
    uartsimSendStr(CTRL_Z);
}

static void commandHelloMagic() {
	uartSendStr("\r\nSending hello magic\r\n");
	const uint8_t buf[(HELLO_LEN * 8 + 6 - 1)/6];
	eproCreateHelloBuffer(buf);
	uartsimSendBuf(buf, sizeof(buf));
	uartsimSendStr(CTRL_Z);
	char buf2[32];
	sprintf(buf2, "buflen=%i\r\n", sizeof(buf));
	uartSendStr(buf2);
}

void tsParseResponse() {
    uartSendStr("Parsing response\r\n");

    uint8_t buf[200];
    int numread = cbufRead(&cbuf, buf, sizeof(buf));

    buf[numread] = '\0';
    uartSendStr("\r\nRaw buf:\r\n[");
    uartSendStr(buf);
    uartSendStr("]\r\n");

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
    uartSendStr("\r\nrespBuf:\r\n[");
    uartSendStr(responseBuffer);
    uartSendStr("]\r\n");
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

static void uartWriteFunc(const SimCommand *c) {
    uartsimSendBuf(c->command, strlen(c->command));
    uartsimSendBuf(c->submitCommand, strlen(c->submitCommand));
}

void tsInitTempStates() {
    cbufInit(&cbuf);
    currentState = 0;
}

static void execCommand (const SimCommand *c) {
    uartSendStr("***execing command ");
    uartSendStr(c->command);
    uartSendStr("\r\n");
    uartWriteFunc(c);
}

void tsRunState() {
    if (currentState >= ASIZE(ALL_COMMANDS)) {
        return;
    }

    const SimCommand *c = ALL_COMMANDS[currentState];
    if (c->comFunc != NULL) {
        c->comFunc();
    } else {
        execCommand(ALL_COMMANDS[currentState]);
    }
}

void tsAddByte(uint8_t b) {
    cbufWrite(&cbuf, &b, 1);
}
