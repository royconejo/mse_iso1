/*
    Copyright 2019 Santiago Germino (royconejo@gmail.com)

    Contibutors:
        {name/email}, {feature/bugfix}.

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
#include "syscall.h"
#include "opaque.h"
#include "runtime.h"
#include "driver/storage.h"
#include "../scheduler.h"
#include "../mutex.h"
#include "../../base/queue.h"
#include "../../base/semaphore.h"
#include "../../base/debug.h"
#include "chip.h"       // CMSIS

#include <string.h>


// Defined in os_ticks.c but not part of the user API.
extern void OS_SetTickHook (OS_TickHook schedulerTickHook);


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


static void taskInitStack (struct OS_TaskControl *task, struct OS_TaskStart *ts)
{
    uint32_t        *stackTop   = &((uint32_t *)task) [(task->stackTop >> 2)];
    OS_TaskReturn   taskReturn  = (task->priority == OS_TaskPriority_Boot)
                                                ? taskBootReturn
                                                : taskCommonReturn;

    // Registers automatically stacked when entering the handler.
    // Values in this stack substitutes those.
    *(--stackTop) = 1 << 24;                    // xPSR.T = 1
    *(--stackTop) = (uint32_t) ts->func;        // xPC
    *(--stackTop) = (uint32_t) taskReturn;      // xLR
    *(--stackTop) = 0;                          // R12
    *(--stackTop) = 0;                          // R3
    *(--stackTop) = 0;                          // R2
    *(--stackTop) = 0;                          // R1
    *(--stackTop) = (uint32_t) ts->param;       // R0
    // LR pushed at interrupt handler. Here artificially set to return to
    // threaded PSP with unused FPU registers (no lazy stacking)
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


static enum OS_Result taskDriverInit (struct OS_TaskControl *task,
                                      struct OS_TaskStart *ts)
{
    switch (task->type)
    {
        case OS_TaskType_DriverStorage:
            return OS_DRIVER_StorageInit (task, ts->initParams);

        default:
            break;
    }

    // Should never get here
    DEBUG_Assert (false);
    return OS_Result_AssertionFailed;
}


static enum OS_Result taskStart (struct OS_TaskStart *ts)
{
    if (!ts || !ts->func || !ts->description)
    {
        return OS_Result_InvalidParams;
    }

    // taskBuffer pointer must be aligned on a 8 byte boundary
    if ((uint32_t)ts->buffer & 0b111)
    {
        return OS_Result_InvalidBufferAlignment;
    }

    // taskBufferSize must be a bare minimum for the task type and also
    // multiple of 4.
    const uint32_t MinBufferSize = OS_TaskMinBufferSize (ts->type,
                                                         ts->initParams);
    if (ts->bufferSize < MinBufferSize || ts->bufferSize & 0b11)
    {
        return OS_Result_InvalidBufferSize;
    }

    memset (ts->buffer, 0, ts->bufferSize);

    struct OS_TaskControl *task = (struct OS_TaskControl *) ts->buffer;

    task->size          = ts->bufferSize;
    task->stackTop      = ts->bufferSize;
    task->description   = ts->description;
    task->startedAt     = OS_TicksUndefined;
    task->terminatedAt  = OS_TicksUndefined;
    task->type          = ts->type;
    task->priority      = ts->priority;
    task->state         = OS_TaskState_Ready;
    task->stackBarrier  = OS_StackBarrierValue;

    SEMAPHORE_Init          (&task->sleep, 1, 1);
    OS_USAGE_CpuReset       (&task->usageCpu);
    OS_USAGE_MemoryReset    (&task->usageMemory);

    if (task->type >= OS_TaskType_Driver__BEGIN
            && task->type <= OS_TaskType_Driver__END)
    {
        enum OS_Result r;
        if ((r = taskDriverInit (task, ts)) != OS_Result_OK)
        {
            return r;
        }
    }

    taskInitStack (task, ts);

    QUEUE_PushNode (&g_OS->tasksReady[task->priority],
                                                (struct QUEUE_Node *) task);
    return OS_Result_OK;
}


static enum OS_Result taskYield ()
{
    OS_SchedulerCallPending ();

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
    OS_SyscallTaskStartInit (&ts, (void *)task, task->size, taskIdle, NULL,
                             OS_TaskPriority_Idle, TaskIdleDescription,
                             OS_TaskType_Generic, NULL);

    if (taskStart (&ts) != OS_Result_OK)
    {
        // Unrecoverable error
        DEBUG_Assert (false);

        while (1)
        {
            __WFI ();
        }
    }

    // Scheduler will choose the next task to be run.
    OS_SchedulerCallPending ();

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
    return (OS_MUTEX_Lock ((struct OS_MUTEX *) sig) == OS_Result_OK);
}


static bool taskSigActionMutexUnlock (void *sig)
{
    return (OS_MUTEX_Unlock ((struct OS_MUTEX *) sig) == OS_Result_OK);
}


static enum OS_Result taskWaitForSignal (struct OS_TaskWaitForSignal *wfs)
{
    if (!wfs
        || !wfs->task
        || !wfs->sigObject
        || wfs->sigType >= OS_TaskSignalType__COUNT)
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
    wfs->task->sigWaitAction    = sigAction;
    wfs->task->sigWaitObject    = wfs->sigObject;
    wfs->task->sigWaitResult    = OS_Result_Waiting;
    wfs->task->suspendedUntil   = (wfs->timeout == OS_WaitForever)
                                        ? OS_WaitForever
                                        : OS_GetTicks() + wfs->timeout;
    OS_SchedulerCallPending ();

    return OS_Result_Waiting;
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

    OS_SchedulerCallPending ();

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

    // Zero ticks causes a lastSuspension reset with current ticks.
    if (*ticks == 0)
    {
        g_OS->currentTask->lastSuspension = OS_GetTicks ();
        return OS_Result_OK;
    }

    g_OS->currentTask->suspendedUntil = g_OS->currentTask->lastSuspension
                                            + *ticks;
    OS_SchedulerCallPending ();

    return OS_Result_OK;
}


// NOTE: Description is compared by pointer address, not pointer contents.
struct OS_TaskControl * taskFind (enum OS_TaskPriority priority,
                                  const char* description)
{
    struct OS_TaskControl   *task;
    struct QUEUE            *queue;

    queue = &g_OS->tasksWaiting[priority];
    for (task = (struct OS_TaskControl *) queue->head; task;
         task = (struct OS_TaskControl *) queue->head->next)
    {
        if (task->description == description)
        {
            return task;
        }
    }

    queue = &g_OS->tasksReady[priority];
    for (task = (struct OS_TaskControl *) queue->head; task;
         task = (struct OS_TaskControl *) queue->head->next)
    {
        if (task->description == description)
        {
            return task;
        }
    }

    task = g_OS->currentTask;
    if (task && task->priority == priority && task->description == description)
    {
        return task;
    }

    return NULL;
}


enum OS_Result taskSleep (struct OS_TaskControl *task)
{
    if (!task)
    {
        return OS_Result_InvalidParams;
    }

    if (!SEMAPHORE_Available (&task->sleep))
    {
        // Already sleeping
        return OS_Result_OK;
    }

    if (DEBUG_Assert (SEMAPHORE_Acquire (&task->sleep)))
    {
        // This new signal adquisition request will keep the task sleeping
        // forever until the semaphore is released.
        struct OS_TaskWaitForSignal wfs;
        wfs.task        = task;
        wfs.sigType     = OS_TaskSignalType_SemaphoreAcquire;
        wfs.sigObject   = &task->sleep;
        wfs.start       = OS_GetTicks ();
        wfs.timeout     = OS_WaitForever;

        return taskWaitForSignal (&wfs);
    }

    return OS_Result_Error;
}


enum OS_Result taskWakeup (struct OS_TaskControl *task)
{
    if (!task)
    {
        return OS_Result_InvalidParams;
    }

    // Note that since its inactive (sleeping), a task cannot wakeup itself.
    // This condition is likely a bug.
    if (!DEBUG_Assert (task != g_OS->currentTask))
    {
        return OS_Result_AssertionFailed;
    }

    // If not awake, release "sleep". On the next scheduling, pending signal
    // adquisition will succeed and this tasks state will change from WAITING
    // to READY. Note that calling this function on an already awake task does
    // not return an error but won't run the scheduler either.
    if (!SEMAPHORE_Available (&task->sleep))
    {
        if (!DEBUG_Assert (SEMAPHORE_Release(&task->sleep)))
        {
            return OS_Result_Error;
        }

        OS_SchedulerCallPending ();
    }

    return OS_Result_OK;
}


enum OS_Result taskDriverStorageAccess (struct OS_TaskDriverStorageAccess *sa)
{
    if (!g_OS->currentTask)
    {
        return OS_Result_NoCurrentTask;
    }

    if (!sa)
    {
        return OS_Result_InvalidParams;
    }

    if (!sa->description || !sa->buf || !sa->count)
    {
        return OS_Result_InvalidParams;
    }

    struct OS_TaskControl *task = taskFind (OS_TaskPriority_DriverStorage,
                                            sa->description);
    if (!task)
    {
        // Requested driver does not exist.
        return OS_Result_NotInitialized;
    }

    sa->result  = OS_Result_NotInitialized;
    sa->sem     = &g_OS->currentTask->sleep;

    enum OS_Result r;

    // Put the caller to sleep on the next scheduler execution.
    // The caller will be waken up by the storage driver when the job is
    // finished -or- the operation timeout has been reached.
    if ((r = taskSleep(g_OS->currentTask)) != OS_Result_OK)
    {
        return r;
    }

    // Add the new job to the driver.
    if ((r = OS_DRIVER_StorageJobAdd(task, sa)) != OS_Result_OK)
    {
        return r;
    }

    // If not already awake, wake up the driver to start a new job.
    if ((r = taskWakeup(task)) != OS_Result_OK)
    {
        return r;
    }

    // Waiting for the new job to be picked up by the driver and be done.
    return OS_Result_Waiting;
}


// Tasks can be abruptly terminated by calling OS_TaskTerminate(TaskBuffer)
// from another task.
static enum OS_Result taskTerminate (struct OS_TaskTerminate *tt)
{
    // A task must have been initiated this operation on itself or another task.
    if (!g_OS->currentTask)
    {
        return OS_Result_NoCurrentTask;
    }

    if (!tt)
    {
        return OS_Result_InvalidParams;
    }

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
        OS_SchedulerCallPending ();
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

    OS_SchedulerCallPending ();

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
            return taskYield ();

        case OS_Syscall_TaskWaitForSignal:
            return taskWaitForSignal ((struct OS_TaskWaitForSignal *) params);

        case OS_Syscall_TaskDelayFrom:
            return taskDelayFrom ((struct OS_TaskDelayFrom *) params);

        case OS_Syscall_TaskPeriodicDelay:
            return taskPeriodicDelay ((OS_Ticks *) params);

        case OS_Syscall_TaskDriverStorageAccess:
            return taskDriverStorageAccess (
                                (struct OS_TaskDriverStorageAccess *) params);

        case OS_Syscall_TaskTerminate:
            return taskTerminate ((struct OS_TaskTerminate *) params);

        case OS_Syscall_Terminate:
            return osTerminate ();

        default:
            break;
    }

    return OS_Result_InvalidParams;
}


// This function can only be called from MSP / Privileged mode.
enum OS_Result OS_SyscallBoot (enum OS_RunMode runMode, OS_Task bootTask,
                               OS_TaskParam bootParam)
{
    if (!OS_RuntimePrivileged ())
    {
        return OS_Result_InvalidCaller;
    }

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
    OS_SyscallTaskStartInit (&ts, g_OS->bootIdleTaskBuffer,
                             sizeof (g_OS->bootIdleTaskBuffer),
                             bootTask, bootParam, OS_TaskPriority_Boot,
                             TaskBootDescription, OS_TaskType_Generic, NULL);

    enum OS_Result r = taskStart (&ts);
    if (r == OS_Result_OK)
    {
        // SysTick won't call the scheduler on its next interrupt
        OS_SchedulerTickBarrier__ACTIVATE ();

        // PSP == 0 signals the first switch from MSP to PSP after
        // initialization.
        __set_PSP (0);

        OS_SetTickHook          (OS_SchedulerTickCallback);
        OS_SchedulerCallPending ();
    }

    return r;
}


// This function can only be called from MSP / Privileged mode.
enum OS_Result OS_SyscallShutdown ()
{
    if (!OS_RuntimePrivileged ())
    {
        return OS_Result_InvalidCaller;
    }

    if (!g_OS)
    {
        return OS_Result_NotInitialized;
    }

    if (g_OS->runMode != OS_RunMode_Finite)
    {
        return OS_Result_InvalidOperation;
    }

    OS_SchedulerTickBarrier__ACTIVATE ();

    OS_SetTickHook (NULL);
    g_OS = NULL;

    OS_SchedulerTickBarrier__CLEAR ();

    return OS_Result_OK;
}


void OS_SyscallTaskStartInit (struct OS_TaskStart *ts, void *buffer,
                              uint32_t bufferSize, OS_Task func,
                              OS_TaskParam param, enum OS_TaskPriority priority,
                              const char *description, enum OS_TaskType type,
                              void *initParams)
{
    DEBUG_Assert (ts);

    ts->buffer      = buffer;
    ts->bufferSize  = bufferSize;
    ts->func        = func;
    ts->param       = param;
    ts->priority    = priority;
    ts->description = description;
    ts->type        = type;
    ts->initParams  = initParams;
}
