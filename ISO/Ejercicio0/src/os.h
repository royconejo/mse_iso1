/*
    Copyright 2019 Santiago Germino (royconejo@gmail.com)

    RETRO-CIAA™ Library - Preemtive multitasking Operating System (RETRO-OS).
                          User API.

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

#include <stdint.h>
#include <stdbool.h>

/*
    RETRO-OS User API
*/

#define OS_TaskReturn(x)    return (x & ((uint32_t) - 1))

typedef uint64_t            OS_TaskRet;
typedef void *              OS_TaskParam;
typedef uint64_t            OS_Ticks;
typedef OS_TaskRet          (*OS_Task) (OS_TaskParam);
typedef void                (*OS_TickHook) (OS_Ticks ticks);


enum OS_Result
{
    OS_Result_OK = 0,
    OS_Result_Timeout,
    OS_Result_AlreadyInitialized,
    OS_Result_NotInitialized,
    OS_Result_NoCurrentTask,
    OS_Result_InvalidParams,
    OS_Result_InvalidBufferAlignment,
    OS_Result_InvalidBufferSize,
    OS_Result_InvalidState
};


enum OS_TaskPriority
{
    OS_TaskPriorityLevel0_Kernel    = 0,
    OS_TaskPriorityLevel1_Kernel,
    OS_TaskPriorityLevel2_Kernel,
    OS_TaskPriorityLevel3_App,
    OS_TaskPriorityLevel4_App,
    OS_TaskPriorityLevel5_App,
    OS_TaskPriorityLevel6_App,
    OS_TaskPriorityLevel7_Idle,
    OS_TaskPriorityLevels,
    OS_TaskPriorityHighest          = OS_TaskPriorityLevel0_Kernel,
    OS_TaskPriorityLowest           = OS_TaskPriorityLevel7_Idle,
    OS_TaskPriority__BEGIN          = OS_TaskPriorityLevel0_Kernel,
    OS_TaskPriority__COUNT          = OS_TaskPriorityLevels
};


enum OS_TaskSignalType
{
    OS_TaskSignalType_SemaphoreAcquire,
    OS_TaskSignalType_SemaphoreRelease,
    OS_TaskSignalType_MutexLock,
    OS_TaskSignalType_MutexUnlock,
    OS_TaskSignalType__COUNT
};


OS_Ticks        OS_GetTicks             ();
void            OS_SetTickHook          (OS_TickHook schedulerTickHook);

uint32_t        OS_InitBufferSize       ();
uint32_t        OS_MinTaskBufferSize    ();

enum OS_Result  OS_Init                 (void *buffer);
void            OS_Forever              () __attribute__((noreturn));
void            OS_Start                ();
bool            OS_Terminate            ();

enum OS_Result  OS_TaskStart            (void *taskBuffer,
                                         uint32_t taskBufferSize,
                                         OS_Task task, void *taskParam,
                                         enum OS_TaskPriority priority,
                                         const char *description);
enum OS_Result  OS_TaskTerminate        (void *taskBuffer, OS_TaskRet retVal);
void *          OS_TaskSelf             ();
enum OS_Result  OS_TaskYield            ();
enum OS_Result  OS_TaskPeriodicDelay    (OS_Ticks ticks);
enum OS_Result  OS_TaskDelayFrom        (OS_Ticks ticks, OS_Ticks from);
enum OS_Result  OS_TaskDelay            (OS_Ticks ticks);
enum OS_Result  OS_TaskWaitForSignal    (enum OS_TaskSignalType sigType,
                                         void *sigObject, OS_Ticks timeout);
enum OS_Result  OS_TaskReturnValue      (void *taskBuffer, uint32_t *retValue);
