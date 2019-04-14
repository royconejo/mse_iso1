/*
    Copyright 2019 Santiago Germino (royconejo@gmail.com)

    RETRO-CIAA™ Library - Preemtive multitasking Operating System (ReTrOS™).
                          System calls.

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
#include "os_syscall.h"
#include "os_private.h"
#include "os_scheduler.h"
#include "os_mutex.h"
#include "queue.h"
#include "semaphore.h"
#include "debug.h"
#include <string.h>
#include "chip.h"       // CMSIS


static OS_TaskRetVal taskIdle (OS_TaskParam arg)
{
    while (1)
    {
        __WFI ();
    }

    return 0;
}


static void taskBootReturn (OS_TaskRetVal retValue)
{
    // A retValue != 0 means an unrecoverable boot error, the OS will be closed
    // (!startedForever) or otherwise get trapped on an infinite loop.
    if (retValue)
    {
        OS_Terminate ();
        while (1)
        {
            __WFI ();
        }
    }

    OS_Syscall (OS_Syscall_TaskBootEnded, NULL);
}


static void taskCommonReturn (OS_TaskRetVal retValue)
{
    struct OS_TaskTerminate tt;
    tt.retVal   = retValue;
    tt.task     = OS_TaskSelf ();

    OS_Syscall (OS_Syscall_TaskTerminate, &tt);
}


static void taskInitStack (struct OS_TaskControl *task, OS_Task taskFunc,
                           void *taskParam, OS_TaskReturn taskReturn)
{
    uint32_t *stackTop = &((uint32_t *)task) [(task->size >> 2)];

    // Registers automatically stacked when entering the handler
    // Values in this stack substitutes those.
    *(--stackTop) = 1 << 24;                    // xPSR.T = 1
    *(--stackTop) = (uint32_t) taskFunc;        // xPC
    *(--stackTop) = (uint32_t) taskReturn;      // xLR
    *(--stackTop) = 0;                          // R12
    *(--stackTop) = 0;                          // R3
    *(--stackTop) = 0;                          // R2
    *(--stackTop) = 0;                          // R1
    *(--stackTop) = (uint32_t) taskParam;       // R0
    // LR pushed at interrupt handler. Here artificially set to return to
    // threaded PSP with FPU registers still unused (no lazy stacking of FPU
    // registers)
    *(--stackTop) = 0xFFFFFFFD;                 // LR IRQ
    // R4-R11 pushed at interrupt handler.
    *(--stackTop) = 0;                          // R11
    *(--stackTop) = 0;                          // R10
    *(--stackTop) = 0;                          // R9
    *(--stackTop) = 0;                          // R8
    *(--stackTop) = 0;                          // R7
    *(--stackTop) = 0;                          // R6
    *(--stackTop) = 0;                          // R5
    *(--stackTop) = 0;                          // R4

    task->sp = (uint32_t) stackTop;
}


static enum OS_Result taskStart (struct OS_TaskStart *ts)
{
    if (!ts || !ts->taskFunc)
    {
        return OS_Result_InvalidParams;
    }

    // taskBuffer pointer must be aligned on a 4 byte boundary
    if ((uint32_t)ts->taskBuffer & 0b11)
    {
        return OS_Result_InvalidBufferAlignment;
    }

    // taskBufferSize must be enough to run the scheduler and must be multiple
    // of 4.
    if (ts->taskBufferSize < OS_TaskMinBufferSize || ts->taskBufferSize & 0b11)
    {
        return OS_Result_InvalidBufferSize;
    }

    if (!ts->description)
    {
        ts->description = TaskNoDescription;
    }

    memset (ts->taskBuffer, 0, ts->taskBufferSize);

    struct OS_TaskControl *task = (struct OS_TaskControl *) ts->taskBuffer;

    task->size           = ts->taskBufferSize;
    task->description    = ts->description;
    task->startedAt      = OS_UndefinedTicks;
    task->terminatedAt   = OS_UndefinedTicks;
    task->priority       = ts->priority;
    task->state          = OS_TaskState_Ready;
    task->stackBarrier   = OS_StackBarrierValue;

    OS_TaskReturn taskReturn = (task->priority == OS_TaskPriority_Boot)
                                                    ? taskBootReturn
                                                    : taskCommonReturn;

    taskInitStack (task, ts->taskFunc, ts->taskParam, taskReturn);

    QUEUE_PushNode (&g_OS->tasksReady[task->priority],
                                                (struct QUEUE_Node *) task);
    return OS_Result_OK;
}


static enum OS_Result taskBootEnded ()
{
    DEBUG_Assert (g_OS->currentTask);

    struct OS_TaskControl *task = g_OS->currentTask;

    // No current task
    g_OS->currentTask   = NULL;

    // Boot task buffer is recycled as the idle task (buffer owned by the OS).
    struct OS_TaskStart ts;
    ts.taskBuffer       = (void *) task;
    ts.taskBufferSize   = task->size;
    ts.taskFunc         = taskIdle;
    ts.taskParam        = NULL;
    ts.priority         = OS_TaskPriority_Idle;
    ts.description      = TaskIdleDescription;

    if (taskStart(&ts) != OS_Result_OK)
    {
        // Unrecoverable error
        DEBUG_Assert (false);

        while (1)
        {
            __WFI ();
        }
    }

    // Scheduler will pick the next task to be run.
    OS_SchedulerWakeup ();

    return OS_Result_OK;
}


// WARNING: Anything inside each one of this functions must be able to be called
//          from the scheduler in handler mode. Not all user API functions
//          are callable from handler mode!.
static bool taskSigActionSemaphoreAcquire (void *sig)
{
    return SEMAPHORE_Acquire ((struct SEMAPHORE *) sig);
}


static bool taskSigActionSemaphoreRelease (void *sig)
{
    return SEMAPHORE_Release ((struct SEMAPHORE *) sig);
}


static bool taskSigActionMutexLock (void *sig)
{
    return OS_MUTEX_Lock ((struct OS_MUTEX *) sig);
}


static bool taskSigActionMutexUnlock (void *sig)
{
    return OS_MUTEX_Unlock ((struct OS_MUTEX *) sig);
}


static enum OS_Result taskWaitForSignal (struct OS_TaskWaitForSignal *wfs)
{
    if (!g_OS->currentTask)
    {
        return OS_Result_NoCurrentTask;
    }

    if (!wfs || !wfs->sigObject || wfs->sigType >= OS_TaskSignalType__COUNT)
    {
        return OS_Result_InvalidParams;
    }

    OS_SigAction sigAction = NULL;
    switch (wfs->sigType)
    {
        case OS_TaskSignalType_SemaphoreAcquire:
            sigAction = taskSigActionSemaphoreAcquire;
            break;
        case OS_TaskSignalType_SemaphoreRelease:
            sigAction = taskSigActionSemaphoreRelease;
            break;
        case OS_TaskSignalType_MutexLock:
            sigAction = taskSigActionMutexLock;
            break;
        case OS_TaskSignalType_MutexUnlock:
            sigAction = taskSigActionMutexUnlock;
            break;
        default:
            DEBUG_Assert (false);
            break;
    }

    DEBUG_Assert (sigAction);

    const bool ActionResult = sigAction (wfs->sigObject);

    // Result is inmediately returned if the action succeded or timeout is zero.
    if (ActionResult || !wfs->timeout)
    {
        return ActionResult? OS_Result_OK : OS_Result_Timeout;
    }

    // Configuring task to be suspended until timeout. In the meantime the
    // scheduler will be retring this signal action.
    g_OS->currentTask->sigWaitAction    = sigAction;
    g_OS->currentTask->sigWaitObject    = wfs->sigObject;
    g_OS->currentTask->sigWaitResult    = OS_Result_NotInitialized;
    g_OS->currentTask->suspendedUntil   = OS_GetTicks() + wfs->timeout;

    OS_SchedulerWakeup ();

    return g_OS->currentTask->sigWaitResult;
}


static enum OS_Result taskDelayFrom (struct OS_TaskDelayFrom *df)
{
    if (!g_OS->currentTask)
    {
        return OS_Result_NoCurrentTask;
    }

    if (!df)
    {
        return OS_Result_InvalidParams;
    }

    g_OS->currentTask->suspendedUntil = df->from + df->ticks;

    OS_SchedulerWakeup ();

    return OS_Result_OK;
}


static enum OS_Result taskPeriodicDelay (OS_Ticks *ticks)
{
    if (!g_OS->currentTask)
    {
        return OS_Result_NoCurrentTask;
    }

    if (!ticks)
    {
        return OS_Result_InvalidParams;
    }

    g_OS->currentTask->suspendedUntil = g_OS->currentTask->lastSuspension
                                            + *ticks;
    OS_SchedulerWakeup ();

    return OS_Result_OK;
}


static enum OS_Result taskTerminate (struct OS_TaskTerminate *tt)
{
    if (!g_OS->currentTask)
    {
        return OS_Result_NoCurrentTask;
    }

    if (!tt)
    {
        return OS_Result_InvalidParams;
    }

    // Tasks can be abruptly terminated by calling OS_TaskTerminate(TaskBuffer)
    // from another task.

    // Own task returning with a call to OS_TaskTerminate (NULL, x).
    if (!tt->task)
    {
        DEBUG_Assert (g_OS->currentTask->state == OS_TaskState_Running);

        tt->task = g_OS->currentTask;
    }
    else if (tt->task->state == OS_TaskState_Terminated)
    {
        // Trying to terminate an already terminated task, do nothing.
        return OS_Result_InvalidState;
    }

    switch (tt->task->state)
    {
        case OS_TaskState_Running:
            DEBUG_Assert (g_OS->currentTask == tt->task);
            g_OS->currentTask = NULL;
            break;

        case OS_TaskState_Ready:
            QUEUE_DetachNode (&g_OS->tasksReady[tt->task->priority],
                                            (struct QUEUE_Node *) tt->task);
            break;

        case OS_TaskState_Waiting:
            QUEUE_DetachNode (&g_OS->tasksWaiting[tt->task->priority],
                                            (struct QUEUE_Node *) tt->task);
            break;

        default:
            // Invalid state
            DEBUG_Assert (false);
            return OS_Result_InvalidState;
    }

    tt->task->retValue      = tt->retVal;
    tt->task->state         = OS_TaskState_Terminated;
    tt->task->terminatedAt  = OS_GetTicks ();

    // If there is no current task, it is assumed that the current running task
    // has been terminated itself (see above), so a scheduler wakeup is needed
    // to assign a new current task. The terminated task stack pointer that
    // called this syscall will no longer be used; it will never receive and
    // process this return value.
    if (!g_OS->currentTask)
    {
        OS_SchedulerWakeup ();
        return OS_Result_OK;
    }

    // Check that there is actually a current task terminating another "task".
    DEBUG_Assert (g_OS->currentTask != tt->task);

    return OS_Result_OK;
}


enum OS_Result osTerminate ()
{
    if (g_OS->runMode != OS_RunMode_Finite)
    {
        // OS cannot be terminated
        return OS_Result_InvalidOperation;
    }

    g_OS->terminatedAt = OS_GetTicks ();

    OS_SchedulerWakeup ();

    // When terminatedAt is given a valid number of ticks, the scheduler will
    // perform a last context switch to recover execution from MSP right after
    // the first context switch at OS_Start(). Then scheduler execution will be
    // terminated along with any defined task, even the caller of this function.
    return OS_Result_OK;
}


enum OS_Result OS_SyscallHandler (enum OS_Syscall call, void *params)
{
    if (!g_OS)
    {
        return OS_Result_NotInitialized;
    }

    switch (call)
    {
        case OS_Syscall_TaskBootEnded:
            return taskBootEnded ();

        case OS_Syscall_TaskStart:
            return taskStart ((struct OS_TaskStart *) params);

        case OS_Syscall_TaskYield:
            OS_SchedulerWakeup ();
            return OS_Result_OK;

        case OS_Syscall_TaskWaitForSignal:
            return taskWaitForSignal ((struct OS_TaskWaitForSignal *) params);

        case OS_Syscall_TaskDelayFrom:
            return taskDelayFrom ((struct OS_TaskDelayFrom *) params);

        case OS_Syscall_TaskPeriodicDelay:
            return taskPeriodicDelay ((OS_Ticks *) params);

        case OS_Syscall_TaskTerminate:
            return taskTerminate ((struct OS_TaskTerminate *) params);

        case OS_Syscall_Terminate:
            return osTerminate ();

        default:
            break;
    }

    return OS_Result_InvalidParams;
}


// This function must only be called from MSP / Privileged mode.
enum OS_Result OS_SyscallBoot (enum OS_RunMode runMode, OS_Task bootTask)
{
    if (!g_OS)
    {
        return OS_Result_NotInitialized;
    }

    if (g_OS->runMode != OS_RunMode_Undefined)
    {
        return OS_Result_InvalidOperation;
    }

    if (runMode == OS_RunMode_Undefined || !bootTask)
    {
        return OS_Result_InvalidParams;
    }

    g_OS->runMode = runMode;

    struct OS_TaskStart ts;
    ts.taskBuffer       = g_OS->bootIdleTaskBuffer;
    ts.taskBufferSize   = sizeof (g_OS->bootIdleTaskBuffer);
    ts.taskFunc         = bootTask;
    ts.taskParam        = NULL;
    ts.priority         = OS_TaskPriority_Boot;
    ts.description      = TaskBootDescription;

    enum OS_Result r = taskStart (&ts);
    if (r == OS_Result_OK)
    {
        // SysTick won't call the scheduler on its next interrupt
        OS_SchedulerTickBarrier__ACTIVATE ();

        // PSP == 0 signals the first switch from msp to psp since
        // initialization.
        __set_PSP (0);

        OS_SetTickHook      (OS_SchedulerTickCallback);
        OS_SchedulerWakeup  ();
    }

    return r;
}
