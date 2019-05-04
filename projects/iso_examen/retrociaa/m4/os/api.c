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
#include "api.h"
#include "scheduler.h"
#include "mutex.h"
#include "private/opaque.h"
#include "private/syscall.h"
#include "private/runtime.h"
#include "private/usage.h"
#include "private/driver/storage.h"
#include "../base/queue.h"
#include "../base/semaphore.h"
#include "../base/debug.h"

#include <string.h>
#include <stdbool.h>


uint32_t OS_InitBufferSize ()
{
    return sizeof (struct OS);
}


uint32_t OS_TaskMinBufferSize (enum OS_TaskType type, void *initParams)
{
    // Bare minimum for a generic task
    uint32_t minBufferSize = OS_TaskGenericMinBufferSize;

    switch (type)
    {
        case OS_TaskType_DriverStorage:
            minBufferSize += OS_DRIVER_StorageBufferSize (initParams);
            break;

        case OS_TaskType_DriverUART:
            break;

        case OS_TaskType_Generic:
            break;

        default:
            // Unknown or unsupported task type
            DEBUG_Assert (false);
            return 0;
    }

    return minBufferSize;
}


enum OS_Result OS_Init (void *buffer)
{
    if (OS_RuntimeTask ())
    {
        return OS_Result_InvalidCaller;
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

    // Highest priority for Systick, second high for SVC and lowest for PendSV.
    // Priorities for external interrupts -like those used for peripherals- must
    // be set above SVCall and below PendSV.
    NVIC_SetPriority (SysTick_IRQn  ,OS_IntPriorityTicks);
    NVIC_SetPriority (SVCall_IRQn   ,OS_IntPrioritySyscall);
    NVIC_SetPriority (PendSV_IRQn   ,OS_IntPriorityScheduler);

    // Enable MCU cycle counter
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    for (int i = OS_TaskPriority__BEGIN; i < OS_TaskPriority__COUNT; ++i)
    {
        QUEUE_Init (&os->tasksReady[i]);
        QUEUE_Init (&os->tasksWaiting[i]);
    }

    os->startedAt       = OS_TicksUndefined;
    os->terminatedAt    = OS_TicksUndefined;
    os->runMode         = OS_RunMode_Undefined;

    OS_USAGE_Init (&os->usage);

    g_OS = os;

    return OS_Result_OK;
}


void OS_Forever (OS_Task bootTask, OS_TaskParam bootParam)
{    
    DEBUG_Assert (!OS_RuntimeTask ());
    DEBUG_Assert (g_OS && bootTask);

    enum OS_Result r = OS_SyscallBoot (OS_RunMode_Forever, bootTask, bootParam);
    DEBUG_Assert (r == OS_Result_OK);

    // OS_Terminate() cannot be called on ON_RunMode_Forever so it should never
    // get here.
    while (1)
    {
        __WFI ();
    }
}


enum OS_Result OS_Start (OS_Task bootTask, OS_TaskParam bootParam)
{
    if (OS_RuntimeTask ())
    {
        return OS_Result_InvalidCaller;
    }

    if (!g_OS)
    {
        return OS_Result_NotInitialized;
    }

    if (!bootTask)
    {
        return OS_Result_InvalidParams;
    }

    enum OS_Result r = OS_SyscallBoot (OS_RunMode_Finite, bootTask, bootParam);
    if (!DEBUG_Assert (r == OS_Result_OK))
    {
        // Unrecoverable error while starting to boot
        return r;
    }
    ////////////////////////////////////////////////////////////////////////////
    // MSP returs to this point when the OS has been instructed to terminate.
    ////////////////////////////////////////////////////////////////////////////
    DEBUG_Assert (!OS_RuntimeTask ());
    DEBUG_Assert (g_OS && g_OS->runMode == OS_RunMode_Finite);

    return OS_SyscallShutdown ();
}


enum OS_Result OS_Terminate ()
{
    if (!OS_RuntimePrivilegedTask ())
    {
        return OS_Result_InvalidCaller;
    }

    return OS_Syscall (OS_Syscall_Terminate, NULL);
}


enum OS_Result OS_TaskStart (void *taskBuffer, uint32_t bufferSize,
                             OS_Task func, OS_TaskParam param,
                             enum OS_TaskPriority priority,
                             const char *description)
{
    if (!OS_RuntimePrivilegedTask ())
    {
        return OS_Result_InvalidCaller;
    }

    if (priority < OS_TaskPriority_KernelHighest
            || priority > OS_TaskPriority_UserLowest)
    {
        return OS_Result_InvalidParams;
    }

    struct OS_TaskStart tc;
    OS_SyscallTaskStartInit (&tc, taskBuffer, bufferSize, func, param,
                             priority, description, OS_TaskType_Generic, NULL);

    return OS_Syscall (OS_Syscall_TaskStart, &tc);
}


enum OS_Result OS_TaskDriverStart (void *taskBuffer, uint32_t bufferSize,
                                   OS_Task func, OS_TaskParam param,
                                   const char *description,
                                   enum OS_TaskType driverType,
                                   void *initParam)
{
    if (!OS_RuntimePrivilegedTask ())
    {
        return OS_Result_InvalidCaller;
    }

    if (driverType < OS_TaskType_Driver__BEGIN
            || driverType > OS_TaskType_Driver__END)
    {
        return OS_Result_InvalidParams;
    }

    // NOTE: correct priority for this driver will be determined by the kernel
    struct OS_TaskStart tc;
    OS_SyscallTaskStartInit (&tc, taskBuffer, bufferSize, func, param,
                             OS_TaskPriority_Idle, description, driverType,
                             initParam);

    return OS_Syscall (OS_Syscall_TaskStart, &tc);
}


enum OS_Result OS_TaskTerminate (void *taskBuffer, OS_TaskRetVal retVal)
{
    if (!OS_RuntimePrivilegedTask ())
    {
        return OS_Result_InvalidCaller;
    }

    struct OS_TaskTerminate tt;
    tt.retVal   = retVal;
    tt.task     = (struct OS_TaskControl *) taskBuffer;

    return OS_Syscall (OS_Syscall_TaskTerminate, &tt);
}


void * OS_TaskSelf ()
{
    if (!DEBUG_Assert (g_OS))
    {
        return NULL;
    }

    return (void *) g_OS->currentTask;
}


enum OS_Result OS_TaskYield ()
{
    if (!OS_RuntimeTask ())
    {
        return OS_Result_InvalidCaller;
    }

    return OS_Syscall (OS_Syscall_TaskYield, NULL);
}

#if 0
enum OS_Result OS_TaskYield ()
{
    if (!OS_RuntimeTask ())
    {
        return OS_Result_InvalidCaller;
    }

    if (OS_RuntimePrivilegedTask ())
    {
        // Shortcut for privileged tasks
        // (No need to call SVC to wakeup the scheduler)
        if (!g_OS)
        {
            return OS_Result_NotInitialized;
        }

        // Only the running task can yield the processor
        DEBUG_Assert (g_OS->currentTask);

        OS_SchedulerWakeUp ();
        return OS_Result_OK;
    }

    return OS_Syscall (OS_Syscall_TaskYield, NULL);
}
#endif

enum OS_Result OS_TaskDelayFrom (OS_Ticks ticks, OS_Ticks from)
{
    if (!OS_RuntimeTask ())
    {
        return OS_Result_InvalidCaller;
    }

    struct OS_TaskDelayFrom df;
    df.ticks    = ticks;
    df.from     = from;

    return OS_Syscall (OS_Syscall_TaskDelayFrom, &df);
}


// Ticks value of zero causes a lastSuspension reset with current ticks.
// Another option to init lastSuspension could be a first call to
// OS_TaskDelay (ticks)
enum OS_Result OS_TaskPeriodicDelay (OS_Ticks ticks)
{
    if (!OS_RuntimeTask ())
    {
        return OS_Result_InvalidCaller;
    }

    return OS_Syscall (OS_Syscall_TaskPeriodicDelay, &ticks);
}


enum OS_Result OS_TaskDelay (OS_Ticks ticks)
{
    return OS_TaskDelayFrom (ticks, OS_GetTicks());
}


enum OS_Result OS_TaskWaitForSignal (enum OS_TaskSignalType sigType,
                                     void *sigObject, OS_Ticks timeout)
{
    if (!OS_RuntimeTask ())
    {
        return OS_Result_InvalidCaller;
    }

    DEBUG_Assert (g_OS->currentTask);

    struct OS_TaskWaitForSignal wfs;
    wfs.task        = OS_TaskSelf ();
    wfs.sigType     = sigType;
    wfs.sigObject   = sigObject;
    wfs.start       = OS_GetTicks ();
    wfs.timeout     = timeout;

    // NOTE: if timeout == 0 an inmediate result will be received, either
    //       OS_Result_OK or OS_Result_Timeout indicating the signal could not
    //       be taken).
    //
    //       if timeout > 0 and the inmediate result was not OK, this task will
    //       be configured to wait for a signal and a PendSV event will be
    //       set to pending to execute the scheduler. This SVC system call
    //       will return the operation last known state (OS_Result_Waiting),
    //       PendSV will be called and this task will be set to WAITING then
    //       switching back here after transitioning to READY as the result of a
    //       timeout or signal adquisition. That is why a final read to the
    //       signal result must be returned to the calling task.
    enum OS_Result r = OS_Syscall (OS_Syscall_TaskWaitForSignal, &wfs);
    if (r != OS_Result_Waiting)
    {
        // Inmediate result
        return r;
    }

    // Deferred result
    return ((struct OS_TaskControl *) OS_TaskSelf ())->sigWaitResult;
}


enum OS_Result OS_TaskSleep (void *taskBuffer)
{
    return OS_Result_Error;
}


enum OS_Result OS_TaskWakeUp (void *taskBuffer)
{
    return OS_Result_Error;
}


enum OS_Result OS_TaskDriverStorage (const char* description,
                                     enum OS_TaskDriverOp op,
                                     uint8_t *buf, uint32_t sector,
                                     uint32_t count)
{
    if (!OS_RuntimeTask ())
    {
        return OS_Result_InvalidCaller;
    }

    struct OS_TaskDriverStorageAccess sa;
    sa.description  = description;
    sa.op           = op;
    sa.buf          = buf;
    sa.sector       = sector;
    sa.count        = count;

    enum OS_Result r = OS_Syscall (OS_Syscall_TaskDriverStorageAccess, &sa);
    if (r != OS_Result_Waiting)
    {
        // Inmediate result
        return r;
    }

    // Deferred result
    return sa.result;
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

