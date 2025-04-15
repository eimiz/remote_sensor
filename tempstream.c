#include <stdio.h>
#include <string.h>
#include "station.h"
#include "wire1.h"
#include "uart.h"
#include "uartsim.h"
#include "eprotocol.h"
#include "tempstates.h"
#include "delay.h"
#include "lcdlogs.h"
#include "eutils.h"
#include "modem.h"
#include "motion.h"

static int packetCounter = 0;
static void tempStreamProc(void *task);
static void tempMeasureProc(void *task);
static void parseEndpointDataResponse();

Task tempStreamTask = {TEMPR_STREAM_EVENT, tempStreamProc, 18000, 0, true};
Task tempMeasureTask = {TEMPR_MEASURE_EVENT, tempMeasureProc, 1500, 0, true};
typedef enum {INIT_STATE, MEASURE_STATE, READ_STATE, CIP_STATE, SEND_STATE, PARSE_ENDPOINT_RESPONSE_STATE} StreamState;
StreamState tempState;

static void measureTemp();
static void readTemp();
static void sendData();
static MODEM_STATUS sendCip();
static TWire1 wire1;
static void tempStreamProc(void *task) {
    stationStopTask(task);
    uartSendLog("stream proc invoked");
    stationStartTask(&tempMeasureTask);
}

static void tempStreamStart() {
    lcdlogsSet(LLOG_STATUS, "Prisijungta");
    tempState = MEASURE_STATE;
    uartSendLog("wire1 init");
    wire1Init(&wire1, &GPIOA, WIRE_PIN);
    uartSendLog("wire1 config");
    wire1Config(&wire1);
    uartSendLog("Starting stream task");
    stationStartTask(&tempStreamTask);
}

void tempStreamReset() {
    tempState = INIT_STATE;
}

void tempStreamStop() {
    stationStopTask(&tempStreamTask);
    stationStopTask(&tempMeasureTask);
}

static void tempStreamAbort() {
    uartSendLog("Reset modem from tempstream");
    tempStreamStop();
    modemUnlock(parseEndpointDataResponse);
    tsResetModemRestartStates();
}


static void parseEndpointDataResponse() {
    const char *responseBuffer = modemGetPart(0);
    if (strlen(responseBuffer) == 0) {
        uartSendLog("Empty respoinse when expecting > or endpoint data");
        tempStreamAbort();
        return;
    }

    if (responseBuffer[0] == '>') {
        uartSendLog("Cip prompt found, ok");
        return;
    }

    //stationReportUartStats();
    uartSendLog("Checking response");
    if (eproCheckResponse(responseBuffer)) {
        tempStreamAbort();
        return;
    }

    stationPostponeTask(&tempStreamTask, tempStreamTask.period);
    tempState = MEASURE_STATE;
    modemUnlock(parseEndpointDataResponse);
}

void tempStreamProcess() {
    uartSendLog("tempStreamProcess");
    switch (tempState) {
        case INIT_STATE:
            tempStreamStart();
            break;
        default:
    }
}


static void tempMeasureProc(void *task) {
    uartSendLog("measure proc invoked");
    char buf[32];
    sprintf(buf, "temp state: %i", tempState);
    switch (tempState) {
        case MEASURE_STATE:
            measureTemp();
            tempState = READ_STATE;
            break;
       case READ_STATE:
            readTemp();
            tempState = CIP_STATE;
            break;
       case CIP_STATE:
            if (!sendCip()) {
                tempState = SEND_STATE;
            }
            break;
       case SEND_STATE:
            uartSendLog("***Send state");
            sendData();
            stationStopTask((Task*)task);
            tempState = MEASURE_STATE;
       default:
    }
}

static MODEM_STATUS sendCip() {
    MODEM_STATUS modemStatus = modemLock(parseEndpointDataResponse);
    if (modemStatus == MODEM_OK) {
        uartSendLog("Sending cip");
        uartsimSendStr("at+cipsend\n");
    } else {
        uartSendLog("Modem locked, no CIP");
    }

    return modemStatus;
}

static void measureTemp() {
    uartSendLog("Measuring temp");
    wire1MeasureTemp(&wire1);
    uartSendLog("After tempMeasure");
}


static void readTemp() {
    uartSendLog("Reading tempr");
    if (wire1ReadTemp(&wire1) != WIRE1_OK) {
        //todo log somewhere
        return;
    }

    uartSendLog("Tempr read ok");
}

static void sendData() {
    uartSendLog("Sending data");
    uint8_t status = 0;
    if ( motionPresent()) {
        status = 1;
        motionReset();
    }

    uint8_t buffer[] = {wire1.tfrac, wire1.tmain, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, status, 17, 18};
    uint8_t encbuffer[ENC_SIZE(NONCE_LEN +  CHA_COUNTER_LEN + sizeof(buffer) + HASH_LEN)];
    char logbuf[32];



    sprintf(logbuf, "encbuf len:%i, raw len: %i", sizeof(encbuffer), NONCE_LEN +  CHA_COUNTER_LEN + sizeof(buffer) + HASH_LEN);
    uartSendLog(logbuf);
    eproCreateDataBuf(encbuffer, buffer, sizeof(buffer));
    uartsimSendBuf(encbuffer, sizeof(encbuffer));
    const char CTRL_Z[] = {26, 0};
    uartsimSendStr(CTRL_Z);
    packetCounter++;
    sprintf(logbuf, "P:%i, rst:%i", packetCounter, tsResetCounter());
    lcdlogsSet(LLOG_STATUS, logbuf);
}
