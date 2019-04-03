/*
    RETRO-CIAA™ Library
    Copyright 2019 Santiago Germino (royconejo@gmail.com)

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

#include "queue.h"
#include "semaphore.h"
#include <stdint.h>
#include <stdbool.h>


typedef uint32_t        OS_TaskRet;
typedef void *          OS_TaskParam;
typedef uint64_t        OS_Ticks;
typedef uint64_t        OS_Cycles;
typedef OS_TaskRet      (*OS_Task) (OS_TaskParam);
typedef void            (*OS_TickHook) (OS_Ticks ticks);


#define OS_IntegerRegisters     17
#define OS_FPointRegisters      (16 + 1 + 16)
#define OS_ContextRegisters     (OS_FPointRegisters + OS_IntegerRegisters)
// NOTE: this number must be big enough for any task to be able to execute the
//       scheduler!
#define OS_MinAppStackSize      128
#define OS_TaskMinBufferSize    (sizeof(struct OS_TaskControl) \
                                            + (OS_ContextRegisters * 4) \
                                            + OS_MinAppStackSize)
#define OS_UndefinedTicks       ((OS_Ticks) -1)
#define OS_StackBarrierValue    0xDEADBEEF


enum OS_Result
{
    OS_Result_OK = 0,
    OS_Result_AlreadyInitialized,
    OS_Result_NotInitialized,
    OS_Result_InvalidParams,
    OS_Result_InvalidBufferAlignment,
    OS_Result_InvalidBufferSize,
    OS_Result_NoCurrentTask
};


enum OS_TaskPriority
{
    OS_TaskPriorityLevel0   = 0,
    OS_TaskPriorityLevel1,
    OS_TaskPriorityLevel2,
    OS_TaskPriorityLevel3,
    OS_TaskPriorityLevel4,
    OS_TaskPriorityLevel5,
    OS_TaskPriorityLevel6,
    OS_TaskPriorityLevel7,
    OS_TaskPriorityLevels,
    OS_TaskPriorityHighest  = OS_TaskPriorityLevel0,
    OS_TaskPriorityLowest   = OS_TaskPriorityLevel6,
    OS_TaskPriorityIdle     = OS_TaskPriorityLevel7,
    OS_TaskPriority__BEGIN  = OS_TaskPriorityLevel0,
    OS_TaskPriority__COUNT  = OS_TaskPriorityLevels
};


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
    uint32_t                retValue;
    OS_Ticks                startedAt;
    OS_Ticks                terminatedAt;
    OS_Ticks                suspendedUntil;
    OS_Ticks                lastSuspension;
    struct SEMAPHORE        *signalWaiting;
    OS_Cycles               cycles;
    enum OS_TaskPriority    priority;
    enum OS_TaskState       state;
    uint32_t                sp;
    uint32_t                stackBarrier;
};


struct OS
{
   OS_Ticks                 startedAt;
   OS_Cycles                schedulerRun;
   // The only one task in the RUNNING state
   struct OS_TaskControl    *currentTask;
   // Tasks in READY state
   struct QUEUE             tasksReady[OS_TaskPriorityLevels];
   // Tasks in any other state
   struct QUEUE             tasksWaiting[OS_TaskPriorityLevels];
   // Idle task buffer (this task is internal and belongs to the OS)
   uint8_t                  idleTaskBuffer[OS_TaskMinBufferSize];
};


enum OS_Result  OS_Init                 (struct OS *o);
enum OS_Result  OS_TaskStart            (void *taskBuffer,
                                         uint32_t taskBufferSize,
                                         OS_Task task, void *taskParam,
                                         enum OS_TaskPriority priority);
enum OS_Result  OS_TaskEnd              (void *taskBuffer, OS_TaskRet retVal);
void *          OS_TaskSelf             ();
enum OS_Result  OS_TaskYield            ();
enum OS_Result  OS_TaskPeriodicDelay    (OS_Ticks ticks);
enum OS_Result  OS_TaskDelayFrom        (OS_Ticks ticks, OS_Ticks from);
enum OS_Result  OS_TaskDelay            (OS_Ticks ticks);
void            OS_Start                () __attribute__((noreturn));
OS_Ticks        OS_GetTicks             ();
void            OS_SetTickHook          (OS_TickHook schedulerTickHook);
