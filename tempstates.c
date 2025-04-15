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
#include "lcdlogs.h"
#include "modem.h"
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


extern char *modemResponseParts[TOKENIZE_MAX_PARTS];

typedef struct {
    CommandFunc comFunc;
    const char *command;
    const char *log;
    const char *submitCommand;
} SimCommand;
const char * const ENDL = "\n";
const char CTRL_Z[] = {26, 0};
const SimCommand  TEST_COMMAND = {NULL, "AT", "Init", ENDL};
const SimCommand  IDENT_COMMAND = {NULL, "ATI", NULL,  ENDL};
const SimCommand  FAKE_COMMAND = {NULL, "ATFFF", NULL, ENDL};
const SimCommand  GPRS_ATTACH = {NULL, "AT+CGATT=1", "Gprs UP", ENDL};
const SimCommand  WIRELESS_UP = {NULL, "AT+CIICR", "Wireless up", ENDL};
const SimCommand  WIRELESS_APN = {NULL, "AT+CSTT=\"ezys\"", "APN", ENDL};
const SimCommand  IPADDR_COMMAND = {NULL, "AT+CIFSR", NULL, ENDL}; //returns only ip address, no OK. Mandatory, otherwise ciat+cipstart won't work
const SimCommand  CONN_COMMAND = {NULL, "at+cipstart=\"TCP\",\"88.223.53.82\",\"44556\"", "Jungiasi", ENDL};
const SimCommand  SEND_COMMAND = {NULL, "at+cipsend", NULL, ENDL};
const SimCommand HELLO_MAGIC = {commandHelloMagic, NULL, "Hello magic", CTRL_Z};
const SimCommand CONSUME_SENTOK_CMD = {commandConsumeSentOk, NULL, NULL, NULL};
const SimCommand CONSUME_NONCEHASH_CMD = {commandConsumeNonceAndHash, NULL, "Inicijavimas 1", CTRL_Z};
const SimCommand SEND_CLIENT_HASH_CMD = {commandSendClientHash, NULL, "Iniciavimas 2", CTRL_Z};

//const SimCommand * const  ALL_COMMANDS[] = {&TEST_COMMAND, &WIRELESS_APN, &WIRELESS_UP, &IPADDR_COMMAND, &CONN_COMMAND, &SEND_COMMAND, &TEXT_COMMAND, &SEND_COMMAND, &TEXT_COMMAND2};
const SimCommand * const  ALL_COMMANDS[] = {&TEST_COMMAND, &WIRELESS_APN, &WIRELESS_UP, &IPADDR_COMMAND, &CONN_COMMAND,
 &SEND_COMMAND, &HELLO_MAGIC, &CONSUME_NONCEHASH_CMD,
 &SEND_COMMAND, &SEND_CLIENT_HASH_CMD };
static int currentState = 0;
static bool running = false;
static int resetCounter = 0;
static void commandConsumeSentOk() {

    //buffer should contain Send OK
    if (strcmp(modemResponseParts[0], "SEND OK")) {
        uartSendLog("got SEND OK");
    } else {
        uartSendLog("Oh no, no SEND OK");
    }
}

static void commandConsumeNonceAndHash() {
    uartSendStr("\r\nProcessing nonce and hash\r\n");
    EproRez rez = eproReadServerNonces(modemResponseParts[0]);
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

int tsResetCounter() {
    return resetCounter;
}

void tsResetModemRestartStates() {
    lcdlogsSet(LLOG_STATUS, "Restarting");
    resetCounter++;
    uartSendLog("Received error, reseting modem");
    stationResetModem();
    delay(10000);
    tsInitTempStates();
    tsStart();
}

void tsProcessResponse() {
    uartSendLog(modemResponseParts[0]);
    if (strcmp(modemResponseParts[0], "ERROR") != 0) {
        currentState = MIN(ASIZE(ALL_COMMANDS), currentState + 1);
    } else {
        tsResetModemRestartStates();
        return;
    }

    char buf[8];
    uartSendStr("Current state is ");
    sprintf(buf, "%i", currentState);
    uartSendLog("last token:");
    uartSendLog(buf);
    if (currentState < ASIZE(ALL_COMMANDS)) {
        tsRunState();
    } else {
        tsStop();
        tempStreamProcess(modemResponseParts[0]);
    }
}

static void uartWriteFunc(const SimCommand *c) {
    uartsimSendBuf(c->command, strlen(c->command));
    uartsimSendBuf(c->submitCommand, strlen(c->submitCommand));
}

static void resetBuf() {
    modemReset();
    currentState = 0;
    tempStreamReset();
}

void tsInitTempStates() {
    //send something to modem to autoconfigure baud rate
    uartsimSendStr("AT");
    uartsimSendStr(ENDL);
    delay(100);
}

void tsStart() {
    resetBuf();
    running = true;
    modemLock(tsProcessResponse);
    tsRunState();
}

static void execCommand (const SimCommand *c) {
    uartSendStr("***execing command ");
    uartSendStr(c->command);
    uartSendStr("\r\n");
    uartWriteFunc(c);
}

void tsStop() {
    running = false;
    modemUnlock(tsProcessResponse);
}

bool tsIsRunning() {
    return running;
}

void tsRunState() {
    if (!running || currentState >= ASIZE(ALL_COMMANDS)) {
        return;
    }

    const SimCommand *c = ALL_COMMANDS[currentState];
    if (c->log != NULL) {
        lcdlogsSet(LLOG_STATUS, c->log);
    }

    if (c->comFunc != NULL) {
        c->comFunc();
    } else {
        execCommand(ALL_COMMANDS[currentState]);
    }
}


