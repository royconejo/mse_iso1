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

#define OS_TaskReturn(x)        return (x & ((uint32_t) - 1))

typedef uint64_t                OS_TaskRet;
typedef void *                  OS_TaskParam;
typedef uint64_t                OS_Ticks;
typedef OS_TaskRet              (*OS_Task) (OS_TaskParam);
typedef void                    (*OS_TickHook) (OS_Ticks ticks);

#define ENUM_FORCE_UINT32(s)    s ## __FORCE32 = ((uint32_t) - 1)


enum OS_Result
{
    ENUM_FORCE_UINT32 (OS_Result),
    OS_Result_OK            = 0,
    OS_Result_InvalidCall   = 1,
    OS_Result_InvalidParams,
    OS_Result_InvalidBufferAlignment,
    OS_Result_InvalidBufferSize,
    OS_Result_InvalidState,
    OS_Result_Timeout,
    OS_Result_AlreadyInitialized,
    OS_Result_NotInitialized,
    OS_Result_NoCurrentTask,
};


enum OS_TaskPriority
{
    ENUM_FORCE_UINT32 (OS_TaskPriority),
    OS_TaskPriority__BEGIN      = 0,
    OS_TaskPriority_DrvHighest  = OS_TaskPriority__BEGIN,
    OS_TaskPriority_Drv0        = OS_TaskPriority_DrvHighest,
    OS_TaskPriority_Drv1,
    OS_TaskPriority_Drv2,
    OS_TaskPriority_DrvLowest   = OS_TaskPriority_Drv2,
    OS_TaskPriority_AppHighest,
    OS_TaskPriority_App0        = OS_TaskPriority_AppHighest,
    OS_TaskPriority_App1,
    OS_TaskPriority_App2,
    OS_TaskPriority_App3,
    OS_TaskPriority_AppLowest   = OS_TaskPriority_App3,
    OS_TaskPriority_Idle,
    OS_TaskPriority__COUNT
};


enum OS_TaskSignalType
{
    ENUM_FORCE_UINT32 (OS_TaskSignalType),
    OS_TaskSignalType_SemaphoreAcquire = 0,
    OS_TaskSignalType_SemaphoreRelease,
    OS_TaskSignalType_MutexLock,
    OS_TaskSignalType_MutexUnlock,
    OS_TaskSignalType__COUNT
};


enum OS_Result  OS_SystemCall           (uint32_t x, uint32_t y, uint32_t z);

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
