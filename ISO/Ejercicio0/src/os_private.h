#pragma once

#include "os.h"
#include "queue.h"
#include <stdint.h>
#include <stdbool.h>


#define OS_StackBarrierValue        0xDEADBEEF
#define OS_IntegerRegisters         17
#define OS_FPointRegisters          (16 + 1 + 16)
#define OS_ContextRegisters         (OS_FPointRegisters + OS_IntegerRegisters)
#define OS_MinAppStackSize          128
#define OS_TaskMinBufferSize        (sizeof(struct OS_TaskControl) \
                                            + (OS_ContextRegisters * 4) \
                                            + OS_MinAppStackSize)
#define OS_UndefinedTicks           ((OS_Ticks) -1)
#define OS_PerfTargetTicks          1000


typedef uint64_t            OS_Cycles;
typedef bool                (*OS_SigAction) (void *sig);
typedef void                (*OS_TaskReturn) (OS_TaskRetVal retVal);


extern struct OS            *g_OS;
extern volatile uint32_t    g_OS_SchedulerTickBarrier;
extern volatile uint32_t    g_OS_SchedulerTicksMissed;
extern const char           *TaskNoDescription;
extern const char           *TaskBootDescription;
extern const char           *TaskIdleDescription;


enum OS_TaskState
{
    OS_TaskState_Terminated,
    OS_TaskState_Waiting,
    OS_TaskState_Ready,
    OS_TaskState_Running
};


struct OS_PerfData
{
    OS_Cycles               curCycles;
    uint32_t                curSwitches;
    OS_Cycles               lastCycles;
    uint32_t                lastSwitches;
    float                   lastUsage;
};


struct OS_TaskControl
{
    struct QUEUE_Node       node;
    uint32_t                size;
    const char              *description;
    uint32_t                retValue;
    OS_Ticks                startedAt;
    OS_Ticks                terminatedAt;
    OS_Ticks                suspendedUntil;
    OS_Ticks                lastSuspension;
    OS_SigAction            sigWaitAction;
    void                    *sigWaitObject;
    enum OS_Result          sigWaitResult;
    OS_Cycles               runCycles;
    struct OS_PerfData      performance;
    enum OS_TaskPriority    priority;
    enum OS_TaskState       state;
    uint32_t                sp;
    uint32_t                stackBarrier;
};


struct OS
{
    OS_Ticks                startedAt;
    OS_Ticks                terminatedAt;
    enum OS_RunMode         runMode;
    OS_Cycles               schedulerRunCycles;
    uint32_t                perfTargetTicksCount;
    uint32_t                perfTargetTicksNext;
    float                   perfCyclesPerTargetTicks;
    bool                    perfNewMeasureBegins;
    struct OS_PerfData      performance;
   // The only one task in the RUNNING state
   struct OS_TaskControl    *currentTask;
   // Tasks in READY state
   struct QUEUE             tasksReady          [OS_TaskPriority__COUNT];
   // Tasks in WAITING state
   struct QUEUE             tasksWaiting        [OS_TaskPriority__COUNT];
   // Boot/Idle task buffer
   uint8_t                  bootIdleTaskBuffer  [OS_TaskMinBufferSize];
};
