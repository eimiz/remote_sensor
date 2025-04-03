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

static int packetCounter = 0;
static void tempStreamProc(void *task);
static void tempMeasureProc(void *task);

Task tempStreamTask = {TEMPR_STREAM_EVENT, tempStreamProc, 20000, 0, true};
Task tempMeasureTask = {TEMPR_MEASURE_EVENT, tempMeasureProc, 1500, 0, true};
typedef enum {INIT_STATE, MEASURE_STATE, READ_STATE, CIP_STATE, SEND_STATE, PARSE_ENDPOINT_RESPONSE_STATE} StreamState;
StreamState tempState;

static void measureTemp();
static void readTemp();
static MODEM_STATUS sendData();
static void sendCip();
static TWire1 wire1;
static void tempStreamProc(void *task) {
    stationStopTask(task);
    uartSendLog("stream proc invoked");
    stationStartTask(&tempMeasureTask);
}

static void tempStreamStart() {
    lcdlogsSet(LLOG_STATUS, "Connected");
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

static void parseEndpointDataResponse() {
    const char *responseBuffer = modemGetPart(0);
    if (strcmp(responseBuffer, "ERROR") == 0) {

    }

    stationReportUartStats();
    uartSendLog("Checking response");
    if (eproCheckResponse(responseBuffer)) {
        uartSendLog("Reset modem from tempstream");
        tempStreamStop();
        modemUnlock(parseEndpointDataResponse);
        tsResetModemRestartStates();
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
            sendCip();
            tempState = SEND_STATE;
            break;
       case SEND_STATE:
            uartSendLog("***Send state");
            if (!sendData()) {
                stationStopTask((Task*)task);
                tempState = MEASURE_STATE;
            }
       default:
    }
}

static void sendCip() {
    uartSendLog("Sending cip");
    uartsimSendStr("at+cipsend\n");
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

static MODEM_STATUS sendData() {
    uartSendLog("Sending data");
    uint8_t buffer[] = {wire1.tfrac, wire1.tmain, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18};
    uint8_t encbuffer[ENC_SIZE(NONCE_LEN +  CHA_COUNTER_LEN + sizeof(buffer) + HASH_LEN)];
    char logbuf[32];
    packetCounter++;
    sprintf(logbuf, "Packets sent:%i", packetCounter);
    lcdlogsSet(LLOG_STATUS, logbuf);

    sprintf(logbuf, "encbuf len:%i, raw len: %i", sizeof(encbuffer), NONCE_LEN +  CHA_COUNTER_LEN + sizeof(buffer) + HASH_LEN);
    uartSendLog(logbuf);
    eproCreateDataBuf(encbuffer, buffer, sizeof(buffer));
    MODEM_STATUS modemStatus = modemLock(parseEndpointDataResponse);
    if (modemStatus == MODEM_OK) {
        uartsimSendBuf(encbuffer, sizeof(encbuffer));
        const char CTRL_Z[] = {26, 0};
        uartsimSendStr(CTRL_Z);
    }

    return modemStatus;
}
