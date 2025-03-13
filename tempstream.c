#include "station.h"
#include "wire1.h"
#include "uart.h"
static void tempStreamProc(void *task);
static void tempMeasureProc(void *task);

Task tempStreamTask = {TEMPR_STREAM_EVENT, tempStreamProc, 12000, 0};
Task tempMeasureTask = {TEMPR_MEASURE_EVENT, tempMeasureProc, 1500, 0};
typedef enum {CONFIG_STATE, MEASURE_STATE, READ_STATE} StreamState;
StreamState tempState = MEASURE_STATE;

static void measureTemp();
static void readTemp();
static TWire1 wire1;
static void tempStreamProc(void *task) {
    stationStartTask(&tempMeasureTask);
}

static void tempMeasureProc(void *task) {
    switch (tempState) {
        case CONFIG_STATE:
            wire1Config(&wire1);
            tempState = MEASURE_STATE;
            break;
        case MEASURE_STATE:
            wire1MeasureTemp(&wire1);
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

