#pragma once

#include "retrociaa/base/mempool.h"
#include "retrociaa/base/uart.h"
#include "retrociaa/os/api.h"
#include "retrociaa/os/mutex.h"


extern const char *g_TaskInputName;
extern const char *g_TaskLedName;
extern const char *g_TaskUartName;


struct Stacks
{
    void                    *taskInput;
    void                    *taskLed;
    void                    *taskUart;
};


struct ButtonTimings
{
    OS_Ticks                fall;
    OS_Ticks                rise;
};


struct LedParams
{
    struct OS_MUTEX         mutex;
    struct UART             uart;
    int                     index;
    OS_Ticks                fallTime;
    OS_Ticks                riseTime;
};


struct Measurements
{
    OS_Ticks                started;
    OS_Ticks                finished;
    struct ButtonTimings    b1;
    struct ButtonTimings    b2;
};


struct AppData
{
    struct MEMPOOL          mempool;
    struct Stacks           stacks;
    struct Measurements     measurements;
    struct LedParams        ledParams;
};

bool            getButtonAction         (bool b, struct ButtonTimings *t);
void            getEdges                (struct Measurements *m,
                                         struct LedParams *lp);
void            measurementsRestart     (struct Measurements *m);
void            sendMessage             (struct LedParams *lp);
enum OS_Result  appDataInit             (struct AppData *ad);
