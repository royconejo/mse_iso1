/*
    Copyright 2019 Santiago Germino (royconejo@gmail.com)

    Contibutors:
        {name/email}, {feature/bugfix}.

    RETRO-CIAA™ Library - Preemtive multitasking Operating System (ReTrOS™).
                          Private (opaque to the user) definitions.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1.  Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.

    2.  Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

    3.  Neither the name of the copyright holder nor the names of its
        contributors may be used to endorse or promote products derived from
        this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once

#include "../api.h"
#include "usage.h"
#include "../../base/queue.h"
#include "../../base/semaphore.h"
#include "../../base/attr.h"

#include <stdint.h>
#include <stdbool.h>


#define OS_StackBarrierValue        0xDEADBEEF
#define OS_IntegerRegisters         17
#define OS_FPointRegisters          (16 + 1 + 16)
#define OS_ContextRegisters         (OS_FPointRegisters + OS_IntegerRegisters)
#define OS_MinAppStackSize          128
#define OS_TaskGenericMinBufferSize (sizeof(struct OS_TaskControl) \
                                            + (OS_ContextRegisters * 4) \
                                            + OS_MinAppStackSize)


typedef bool                (*OS_SigAction) (void *sig);
typedef void                (*OS_TaskReturn) (OS_TaskRetVal retVal);


extern struct OS            *g_OS;
extern volatile uint32_t    g_OS_SchedulerCallPending;
extern volatile uint32_t    g_OS_SchedulerTickBarrier;
extern volatile uint32_t    g_OS_SchedulerTicksMissed;
extern const char           *TaskBootDescription;
extern const char           *TaskIdleDescription;


enum OS_TaskState
{
    OS_TaskState_Terminated,
    OS_TaskState_Waiting,
    OS_TaskState_Ready,
    OS_TaskState_Running
};


struct OS_TaskControl
{
    struct QUEUE_Node       node;
    uint32_t                size;
    uint32_t                stackTop;
    const char              *description;
    uint32_t                retValue;
    OS_Ticks                startedAt;
    OS_Ticks                terminatedAt;
    OS_Ticks                suspendedUntil;
    OS_Ticks                lastSuspension;
    OS_SigAction            sigWaitAction;
    void                    *sigWaitObject;
    enum OS_Result          sigWaitResult;
    struct SEMAPHORE        sleep;
    enum OS_TaskType        type;
    enum OS_TaskPriority    priority;
    enum OS_TaskState       state;
    OS_Cycles               runCycles;
    struct OS_USAGE_Cpu     usageCpu;
    struct OS_USAGE_Memory  usageMemory;
    uint32_t                sp;
    uint32_t                stackBarrier;
}
ATTR_DataAlign8;


struct OS
{
    OS_Ticks                startedAt;
    OS_Ticks                terminatedAt;
    enum OS_RunMode         runMode;
    OS_Cycles               runCycles;
    struct OS_USAGE         usage;
    struct OS_USAGE_Cpu     usageCpu;
    // The only RUNNING task
    struct OS_TaskControl   *currentTask;
    // Tasks in READY state
    struct QUEUE            tasksReady          [OS_TaskPriority__COUNT];
    // Tasks in WAITING state
    struct QUEUE            tasksWaiting        [OS_TaskPriority__COUNT];
    // Boot/Idle task buffer
    uint8_t                 bootIdleTaskBuffer  [OS_TaskGenericMinBufferSize]
                                                ATTR_DataAlign8;
}
ATTR_DataAlign8;
