/*
    Copyright 2019 Santiago Germino (royconejo@gmail.com)

    RETRO-CIAA™ Library - Preemtive multitasking Operating System (ReTrOS™).
                          User API and scheduler implementation.

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
#include "os.h"
#include "os_private.h"
#include "os_scheduler.h"
#include "os_syscall.h"
#include "os_runtime.h"
#include "os_mutex.h"
#include "queue.h"
#include "semaphore.h"
#include "debug.h"
#include <string.h>
#include <stdbool.h>
#include "chip.h"   // CMSIS

#ifndef RETROCIAA_OS_CUSTOM_SYSTICK
#include "systick.h"
#endif


#ifdef RETROCIAA_SYSTICK
inline OS_Ticks OS_GetTicks ()
{
    return SYSTICK_Now ();
}


inline void OS_SetTickHook (OS_TickHook schedulerTickHook)
{
    SYSTICK_SetHook (schedulerTickHook);
}
#endif


uint32_t OS_InitBufferSize ()
{
    return sizeof (struct OS);
}


uint32_t OS_MinTaskBufferSize ()
{
    return OS_TaskMinBufferSize;
}


enum OS_Result OS_Init (void *buffer)
{
    if (OS_RuntimeTask ())
    {
        return OS_Result_InvalidCall;
    }

    if (g_OS)
    {
        return OS_Result_AlreadyInitialized;
    }

    if (!buffer)
    {
        return OS_Result_InvalidParams;
    }

    struct OS *os = (struct OS *) buffer;

    memset (os, 0, sizeof(struct OS));

    // Highest priority for Systick, second high for SVC and third to PendSV.
    // This is a design choice.
    NVIC_SetPriority (SysTick_IRQn  ,0);
    NVIC_SetPriority (SVCall_IRQn   ,1);
    NVIC_SetPriority (PendSV_IRQn   ,2);

    // Enable MCU cycle counter
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    for (int i = OS_TaskPriority__BEGIN; i < OS_TaskPriority__COUNT; ++i)
    {
        QUEUE_Init (&os->tasksReady[i]);
        QUEUE_Init (&os->tasksWaiting[i]);
    }

    os->startedAt                   = OS_UndefinedTicks;
    os->terminatedAt                = OS_UndefinedTicks;
    os->runMode                     = OS_RunMode_Undefined;
    os->perfTargetTicksCount        = OS_PerfTargetTicks;
    // NOTE: a Tick rate of 1/1000 of a second is assumed!
    // 1000 - 100 (percent) = 10
    os->perfCyclesPerTargetTicks    = 1.0f / ((float) SystemCoreClock
                                            / (float) os->perfTargetTicksCount
                                            * 10.0f);
    g_OS = os;

    return OS_Result_OK;
}


void OS_Forever (OS_Task bootTask)
{    
    DEBUG_Assert (!OS_RuntimeTask ());
    DEBUG_Assert (g_OS && bootTask);

    enum OS_Result r = OS_SyscallBoot (OS_RunMode_Forever, bootTask);
    DEBUG_Assert (r == OS_Result_OK);

    // OS_Terminate() cannot be called on ON_RunMode_Forever so it should never
    // get here.
    while (1)
    {
        __WFI ();
    }
}


enum OS_Result OS_Start (OS_Task bootTask)
{
    if (OS_RuntimeTask ())
    {
        return OS_Result_InvalidCall;
    }

    if (!g_OS)
    {
        return OS_Result_NotInitialized;
    }

    if (!bootTask)
    {
        return OS_Result_InvalidParams;
    }

    enum OS_Result r = OS_SyscallBoot (OS_RunMode_Finite, bootTask);
    if (r != OS_Result_OK)
    {
        // Unrecoverable error while starting to boot
        DEBUG_Assert (false);
        return r;
    }
    ////////////////////////////////////////////////////////////////////////////
    // if MSP returs to this point it means the OS has been instructed to
    // terminate.
    ////////////////////////////////////////////////////////////////////////////
    DEBUG_Assert (!OS_RuntimeTask ());
    DEBUG_Assert (g_OS && g_OS->runMode == OS_RunMode_Finite);

    OS_SchedulerTickBarrier__ACTIVATE ();

    OS_SetTickHook (NULL);
    g_OS = NULL;

    OS_SchedulerTickBarrier__CLEAR ();

    return OS_Result_OK;
}


enum OS_Result OS_Terminate ()
{
    if (!OS_RuntimePrivilegedTask ())
    {
        return OS_Result_InvalidCall;
    }

    return OS_Syscall (OS_Syscall_Terminate, NULL);
}


enum OS_Result OS_TaskStart (void *taskBuffer, uint32_t taskBufferSize,
                             OS_Task taskFunc, void *taskParam,
                             enum OS_TaskPriority priority,
                             const char *description)
{
    if (!OS_RuntimePrivilegedTask ())
    {
        return OS_Result_InvalidCall;
    }

    if (priority < OS_TaskPriority_DrvHighest
            || priority > OS_TaskPriority_AppLowest)
    {
        return OS_Result_InvalidParams;
    }

    struct OS_TaskStart ts;
    ts.taskBuffer       = taskBuffer;
    ts.taskBufferSize   = taskBufferSize;
    ts.taskFunc         = taskFunc;
    ts.taskParam        = taskParam;
    ts.priority         = priority;
    ts.description      = description;

    return OS_Syscall (OS_Syscall_TaskStart, &ts);
}


enum OS_Result OS_TaskTerminate (void *taskBuffer, OS_TaskRetVal retVal)
{
    if (!OS_RuntimePrivilegedTask ())
    {
        return OS_Result_InvalidCall;
    }

    struct OS_TaskTerminate tt;
    tt.retVal   = retVal;
    tt.task     = (struct OS_TaskControl *) taskBuffer;

    return OS_Syscall (OS_Syscall_TaskTerminate, &tt);
}


void * OS_TaskSelf ()
{
    if (!g_OS)
    {
        return NULL;
    }

    return (void *) g_OS->currentTask;
}


enum OS_Result OS_TaskYield ()
{
    if (!OS_RuntimeTask ())
    {
        return OS_Result_InvalidCall;
    }

    if (OS_RuntimePrivilegedTask ())
    {
        // Shortcut for privileged tasks
        // (No need to call SVC to wakeup the scheduler)
        if (!g_OS)
        {
            return OS_Result_NotInitialized;
        }

        // Only the already running task can yield the processor
        DEBUG_Assert (g_OS->currentTask);

        OS_SchedulerWakeup ();
        return OS_Result_OK;
    }

    return OS_Syscall (OS_Syscall_TaskYield, NULL);
}


enum OS_Result OS_TaskDelayFrom (OS_Ticks ticks, OS_Ticks from)
{
    struct OS_TaskDelayFrom df;
    df.ticks    = ticks;
    df.from     = from;

    return OS_Syscall (OS_Syscall_TaskDelayFrom, &df);
}


enum OS_Result OS_TaskPeriodicDelay (OS_Ticks ticks)
{
    return OS_Syscall (OS_Syscall_TaskPeriodicDelay, &ticks);
}


enum OS_Result OS_TaskDelay (OS_Ticks ticks)
{
    return OS_TaskDelayFrom (ticks, OS_GetTicks());
}


enum OS_Result OS_TaskWaitForSignal (enum OS_TaskSignalType sigType,
                                     void *sigObject, OS_Ticks timeout)
{
    struct OS_TaskWaitForSignal wfs;
    wfs.sigType     = sigType;
    wfs.sigObject   = sigObject;
    wfs.start       = OS_GetTicks ();
    wfs.timeout     = timeout;

    return OS_Syscall (OS_Syscall_TaskWaitForSignal, &wfs);
}


enum OS_Result OS_TaskReturnValue (void *taskBuffer, uint32_t *retValue)
{
    if (!taskBuffer || !retValue)
    {
        return OS_Result_InvalidParams;
    }

    struct OS_TaskControl *task = (struct OS_TaskControl *) taskBuffer;

    if (task->stackBarrier != OS_StackBarrierValue)
    {
        return OS_Result_InvalidBuffer;
    }

    if (task->state != OS_TaskState_Terminated)
    {
        return OS_Result_InvalidState;
    }

    *retValue = task->retValue;

    return OS_Result_OK;
}

