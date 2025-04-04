#pragma once
#include <stdint.h>
#include <stdbool.h>
typedef enum {TEMPR_EVENT = 0,  MOTION_EVENT, CHECKCHARGE_EVENT, BLINK_EVENT, BLINK2_EVENT, BLINK3_EVENT, SIMPROCESS_EVENT, SIMRX_WATCH_EVENT, TEMPR_STREAM_EVENT, TEMPR_MEASURE_EVENT, AUTOSTART_EVENT, LINK_QUALITY_EVENT, SERVICE_PROVIDER_EVENT} TEvent;

typedef void (*TaskFunc)(void *);
typedef struct {
   const TEvent event;
   const TaskFunc func;
   //when period = 0 - EVENT must be set manually to execute the task
    uint32_t period;
    uint32_t lastTick;
    bool active;
} Task;

void stationStartTask(Task *task);
void stationStopTask(Task *task);
void stationPostponeTask(Task *task, uint32_t period);
void stationResetModem();
void stationReportUartStats();
