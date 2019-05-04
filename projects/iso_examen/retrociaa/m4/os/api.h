/*
    Copyright 2019 Santiago Germino (royconejo@gmail.com)

    Contibutors:
        {name/email}, {feature/bugfix}.

    RETRO-CIAA™ Library - Preemtive multitasking Operating System (ReTrOS™).
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

#include "port/ticks.h"
#include "../base/attr.h"

#include "chip.h"   // CMSIS
#include <stdint.h>


#define OS_IntPriorityTicks         0
#define OS_IntPrioritySyscall       1
#define OS_IntPriorityScheduler     ((1 << __NVIC_PRIO_BITS) - 1)


typedef uint32_t            OS_TaskRetVal;
typedef void *              OS_TaskParam;
typedef OS_TaskRetVal       (*OS_Task) (OS_TaskParam);


enum OS_Result
{
    ATTR_EnumForceUint32 (OS_Result),
    OS_Result_OK                    = 0,
    OS_Result_Error                 = 1,
    OS_Result_InvalidCaller,
    OS_Result_InvalidParams,
    OS_Result_InvalidBuffer,
    OS_Result_InvalidBufferAlignment,
    OS_Result_InvalidBufferSize,
    OS_Result_InvalidState,
    OS_Result_InvalidOperation,
    OS_Result_Timeout,
    OS_Result_Waiting,
    OS_Result_Retry,
    OS_Result_BufferFull,
    OS_Result_Empty,
    OS_Result_Locked,
    OS_Result_AlreadyInitialized,
    OS_Result_NotInitialized,
    OS_Result_NoCurrentTask,
    OS_Result_AssertionFailed
};


enum OS_RunMode
{
    OS_RunMode_Undefined            = 0,
    OS_RunMode_Forever,
    OS_RunMode_Finite
};


enum OS_TaskPriority
{
//    ATTR_EnumForceUint32 (OS_TaskPriority),
    OS_TaskPriority_Boot            = 0,
    OS_TaskPriority_Kernel0,
    OS_TaskPriority_Kernel1,
    OS_TaskPriority_Kernel2,
    OS_TaskPriority_User0,
    OS_TaskPriority_User1,
    OS_TaskPriority_User2,
    OS_TaskPriority_Idle,
    OS_TaskPriority__COUNT,
    OS_TaskPriority__BEGIN          = OS_TaskPriority_Boot,
    OS_TaskPriority_KernelHighest   = OS_TaskPriority_Kernel0,
    OS_TaskPriority_KernelLowest    = OS_TaskPriority_Kernel2,
    OS_TaskPriority_UserHighest     = OS_TaskPriority_User0,
    OS_TaskPriority_UserLowest      = OS_TaskPriority_User2,
    OS_TaskPriority_DriverStorage   = OS_TaskPriority_Kernel2,
    OS_TaskPriority_DriverComUART   = OS_TaskPriority_Kernel2
};


enum OS_TaskType
{
    OS_TaskType_Generic             = 0,
    OS_TaskType_DriverStorage,
    OS_TaskType_DriverUART,
    OS_TaskType__COUNT,
    OS_TaskType__BEGIN              = OS_TaskType_Generic,
    OS_TaskType_Driver__BEGIN       = OS_TaskType_DriverStorage,
    OS_TaskType_Driver__END         = OS_TaskType_DriverUART
};


enum OS_TaskSignalType
{
    ATTR_EnumForceUint32 (OS_TaskSignalType),
    OS_TaskSignalType_SemaphoreAcquire = 0,
    OS_TaskSignalType_SemaphoreRelease,
    OS_TaskSignalType_MutexLock,
    OS_TaskSignalType_MutexUnlock,
    OS_TaskSignalType__COUNT
};


uint32_t        OS_InitBufferSize       ();
uint32_t        OS_TaskMinBufferSize    (enum OS_TaskType type,
                                         void *initParams);

enum OS_Result  OS_Init                 (void *buffer);
void            OS_Forever              (OS_Task bootTask,
                                         OS_TaskParam bootParam)
                                                            ATTR_NeverReturn;
enum OS_Result  OS_Start                (OS_Task bootTask,
                                         OS_TaskParam bootParam);
enum OS_Result  OS_Terminate            ();

enum OS_Result  OS_TaskStart            (void *taskBuffer, uint32_t bufferSize,
                                         OS_Task func, OS_TaskParam param,
                                         enum OS_TaskPriority priority,
                                         const char *description);
enum OS_Result  OS_TaskDriverStart      (void *taskBuffer, uint32_t bufferSize,
                                         OS_Task func, OS_TaskParam param,
                                         const char *description,
                                         enum OS_TaskType driverType,
                                         void *initParam);
enum OS_Result  OS_TaskTerminate        (void *taskBuffer,
                                         OS_TaskRetVal retVal);

void *          OS_TaskSelf             ();
enum OS_Result  OS_TaskYield            ();
enum OS_Result  OS_TaskPeriodicDelay    (OS_Ticks ticks);
enum OS_Result  OS_TaskDelayFrom        (OS_Ticks ticks, OS_Ticks from);
enum OS_Result  OS_TaskDelay            (OS_Ticks ticks);
enum OS_Result  OS_TaskWaitForSignal    (enum OS_TaskSignalType sigType,
                                         void *sigObject, OS_Ticks timeout);
enum OS_Result  OS_TaskSleep            (void *taskBuffer);
enum OS_Result  OS_TaskWakeup           (void *taskBuffer);
enum OS_Result  OS_TaskReturnValue      (void *taskBuffer, uint32_t *retValue);
