#include <stdio.h>
#include "station.h"
#include "wire1.h"
#include "uart.h"
#include "uartsim.h"
#include "eprotocol.h"

static void tempStreamProc(void *task);
static void tempMeasureProc(void *task);

Task tempStreamTask = {TEMPR_STREAM_EVENT, tempStreamProc, 12000, 0, true};
Task tempMeasureTask = {TEMPR_MEASURE_EVENT, tempMeasureProc, 1500, 0, true};
typedef enum {INIT_STATE, MEASURE_STATE, READ_STATE, CIP_STATE, SEND_STATE, PARSE_ENDPOINT_RESPONSE_STATE} StreamState;
StreamState tempState;

static void measureTemp();
static void readTemp();
static void sendData();
static void sendCip();
static TWire1 wire1;
static void tempStreamProc(void *task) {
    uartSendLog("stream proc invoked");
    stationStartTask(&tempMeasureTask);
}

static void tempStreamStart() {
    tempState = MEASURE_STATE;

    uartSendLog("wire1 init");
    wire1Init(&wire1, &GPIOA, WIRE_PIN);
    uartSendLog("wire1 config");
    wire1Config(&wire1);
    uartSendLog("Starting stream task");
    stationStartTask(&tempStreamTask);
}

void tempStreamProcess(const uint8_t *responseBuffer) {
    uartSendLog("tempStreamProcess");
    switch (tempState) {
        case INIT_STATE:
            tempStreamStart();
            break;
        case PARSE_ENDPOINT_RESPONSE_STATE:
            uartSendLog("Checking response");
            eproCheckResponse(responseBuffer);
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
            sendData();
            stationStopTask((Task*)task);
            tempState = PARSE_ENDPOINT_RESPONSE_STATE;
       default:
    }
}

static void sendCip() {
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

static void sendData() {
    uartSendLog("Sending data");
    uint8_t buffer[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18};
    uint8_t encbuffer[ENC_SIZE(NONCE_LEN +  CHA_COUNTER_LEN + sizeof(buffer) + HASH_LEN)];
    char logbuf[32];
    sprintf(logbuf, "encbuf len:%i, raw len: %i", sizeof(encbuffer), NONCE_LEN +  CHA_COUNTER_LEN + sizeof(buffer) + HASH_LEN);
    uartSendLog(logbuf);
    eproCreateDataBuf(encbuffer, buffer, sizeof(buffer));
    uartsimSendBuf(encbuffer, sizeof(encbuffer));
    uartsimSend(26);
    stationStopTask(&tempStreamTask);
}

