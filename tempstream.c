#include "station.h"
#include "wire1.h"
#include "uart.h"
static void tempStreamProc(void *task);
static void tempMeasureProc(void *task);

Task tempStreamTask = {TEMPR_STREAM_EVENT, tempStreamProc, 12000, 0, true};
Task tempMeasureTask = {TEMPR_MEASURE_EVENT, tempMeasureProc, 1500, 0, true};
typedef enum {MEASURE_STATE, READ_STATE} StreamState;
StreamState tempState = MEASURE_STATE;

static void measureTemp();
static void readTemp();
static TWire1 wire1;
static void tempStreamProc(void *task) {
    uartSendLog("stream proc invoked");
    stationStartTask(&tempMeasureTask);
}

static void measureTemp() {
    uartSendLog("Measuring temp");
    wire1MeasureTemp(&wire1);
    uartSendLog("After tempMeasure");
}

static void tempMeasureProc(void *task) {
    uartSendLog("measure proc invoked");
    switch (tempState) {
        case MEASURE_STATE:
            measureTemp();
            tempState = READ_STATE;
            break;
       case READ_STATE:
            readTemp();
            tempState = MEASURE_STATE;
            stationStopTask((Task*)task);
            break;
       default:
    }
}

void tempStreamStart() {
    uartSendLog("wire1 init");
    wire1Init(&wire1, &GPIOA, WIRE_PIN);
    uartSendLog("wire1 config");
    wire1Config(&wire1);
    uartSendLog("Starting stream task");
    stationStartTask(&tempStreamTask);
}

static void readTemp() {
    uartSendLog("Reading tempr");
    if (wire1ReadTemp(&wire1) != WIRE1_OK) {
        //todo log somewhere
        return;
    }

    uartSendLog("Tempr read ok");
}

