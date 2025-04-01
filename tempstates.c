#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "delay.h"
#include "circbuf.h"
#include "uart.h"
#include "uartsim.h"
#include "tempstates.h"
#include "eprotocol.h"
#include "tempstream.h"
#include "station.h"
#include "tokenize.h"

#define ASIZE(x) (sizeof(x) / sizeof(x[0]))
#define MIN(x,y) ((x) < (y)) ? (x) : (y)
#define MAX(x,y) ((x) > (y)) ? (x) : (y)

#define NONCE_LEN 12

static void commandHelloMagic();
static void commandConsumeSentOk();
static void commandConsumeNonceAndHash();
static void commandSendClientHash();
typedef void (*CommandFunc)();
typedef enum {STATE_INIT, STATE_GPRS_INIT, STATE_CONNECTING, STATE_READY} ClState;
char bigbuf[200];
char *responseParts[10];

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
static bool running = false;
static void commandConsumeSentOk() {
    //buffer should contain Send OK
    if (strcmp(responseParts[0], "SEND OK")) {
        uartSendLog("got SEND OK");
    } else {
        uartSendLog("Oh no, no SEND OK");
    }
}

static void commandConsumeNonceAndHash() {
    uartSendStr("\r\nProcessing nonce and hash\r\n");
    EproRez rez = eproReadServerNonces(responseParts[0]);
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
    uint8_t outbuffer[ENC_SIZE(CLIENT_HASH_LEN) ];
    uartSendLog("creating client hash");
    eproCreateClientHash(outbuffer);
    uartSendLog("sending client hash");
    uartsimSendBuf(outbuffer, sizeof(outbuffer));
    uartsimSendStr(CTRL_Z);
}

static void commandHelloMagic() {
	uartSendStr("\r\nSending hello magic\r\n");
	const uint8_t buf[ENC_SIZE(HELLO_LEN)];
	eproCreateHelloBuffer(buf);
	uartsimSendBuf(buf, sizeof(buf));
	uartsimSendStr(CTRL_Z);
	char buf2[32];
	sprintf(buf2, "buflen=%i\r\n", sizeof(buf));
	uartSendStr(buf2);
}

void tsParseResponse() {
    uartSendStr("Parsing response\r\n");


    int numread = cbufRead(&cbuf, bigbuf, sizeof(bigbuf));

    bigbuf[numread] = '\0';
    uartSendLog("Raw buf:");
    uartSendLog(bigbuf);

    if (numread <= 0) {
        strcpy(bigbuf, "wrong response");
    }

    tokenize(responseParts, bigbuf);
}

void tsResetModemRestartStates() {
        uartSendLog("Received error, reseting modem");
        stationResetModem();
        delay(10000);
        tsInitTempStates();
        tsRunState();
}

void tsProcessResponse() {
    tsParseResponse();
    uartSendLog(responseParts[0]);
    if (strcmp(responseParts[0], "ERROR") != 0) {
        currentState = MIN(ASIZE(ALL_COMMANDS), currentState + 1);
    } else {
        tsResetModemRestartStates();
        return;
    }

    char buf[8];
    uartSendStr("Current state is ");
    sprintf(buf, "%i", currentState);
    uartSendLog(buf);
    if (currentState < ASIZE(ALL_COMMANDS)) {
        tsRunState();
    } else {
        tempStreamProcess(responseParts[0]);
    }
}

static void uartWriteFunc(const SimCommand *c) {
    uartsimSendBuf(c->command, strlen(c->command));
    uartsimSendBuf(c->submitCommand, strlen(c->submitCommand));
}

void tsInitTempStates() {
    cbufInit(&cbuf);
    currentState = 0;
    tempStreamReset();
    //send something to modem to autoconfigure baud rate
    uartsimSendStr("AT");
    uartsimSendStr(ENDL);
}

static void execCommand (const SimCommand *c) {
    uartSendStr("***execing command ");
    uartSendStr(c->command);
    uartSendStr("\r\n");
    uartWriteFunc(c);
}

void tsSetRunning(bool r) {
    running = r;
}

bool tsIsRunning() {
    return running;
}

void tsRunState() {
    if (!running || currentState >= ASIZE(ALL_COMMANDS)) {
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
