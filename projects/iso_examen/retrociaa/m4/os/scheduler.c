/*
    Copyright 2019 Santiago Germino (royconejo@gmail.com)

    Contibutors:
        {name/email}, {feature/bugfix}.

    RETRO-CIAA™ Library - Preemtive multitasking Operating System (ReTrOS™).
                          Scheduler functions.

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
#include "scheduler.h"
#include "private/opaque.h"
#include "private/runtime.h"
#include "private/usage.h"
#include "../base/debug.h"
#include "chip.h"       // CMSYS


inline static void taskSigWaitEnd (struct OS_TaskControl *tc,
                                   enum OS_Result result)
{
    tc->sigWaitAction = NULL;
    tc->sigWaitObject = NULL;
    tc->sigWaitResult = result;
}


static void taskUpdateState (struct OS_TaskControl *tc, const OS_Ticks Now)
{
    // Not waiting for anything
    if (!tc->suspendedUntil)
    {
        tc->state = OS_TaskState_Ready;
        return;
    }


    // "Suspended until" still in the future.
    if (tc->suspendedUntil > Now)
    {
        // From now on start by assuming a waiting state
        tc->state = OS_TaskState_Waiting;

        // And no signal to test, just keep waiting.
        if (!tc->sigWaitAction)
        {
            return;
        }

        DEBUG_Assert (tc->sigWaitObject);
        // Do the signal test
        {
            // Trick to be able to get a correct OS_TaskSelf () inside a signal
            // action run by the scheduler.
            struct OS_TaskControl *ctb = g_OS->currentTask;
            g_OS->currentTask = tc;

            // if the signal the task was waiting for can be adquired then
            // switch it to a ready state.
            if (tc->sigWaitAction (tc->sigWaitObject))
            {
                taskSigWaitEnd (tc, OS_Result_OK);
                tc->suspendedUntil  = 0;
                tc->state           = OS_TaskState_Ready;
            }

            g_OS->currentTask = ctb;
        }
        return;
    }


    // Last case: "suspendedUntil" no longer in the future.
    DEBUG_Assert (tc->suspendedUntil <= Now);

    if (!tc->sigWaitAction)
    {
        // Not waiting for a signal, common delay is over.
        tc->lastSuspension = tc->suspendedUntil;
    }
    else
    {
        // A signal action has been timed out.
        taskSigWaitEnd (tc, OS_Result_Timeout);
    }

    tc->suspendedUntil  = 0;
    tc->state           = OS_TaskState_Ready;
}


inline static void schedulerUpdateWaitingTasks (const OS_Ticks Now)
{
    for (int i = OS_TaskPriority__BEGIN; i < OS_TaskPriority__COUNT; ++i)
    {
        struct OS_TaskControl *task;

        for (task = (struct OS_TaskControl *) g_OS->tasksWaiting[i].head; task;
             task = (struct OS_TaskControl *) task->node.next)
        {
            DEBUG_Assert (task->stackBarrier == OS_StackBarrierValue);

            taskUpdateState (task, Now);

            if (task->state == OS_TaskState_Ready)
            {
                // Timeout reached. This now ready task will be moved to its
                // corresponding task list.
                QUEUE_DetachNode (&g_OS->tasksWaiting[i],
                                                (struct QUEUE_Node *) task);
                QUEUE_PushNode   (&g_OS->tasksReady[i],
                                                (struct QUEUE_Node *) task);
            }

            DEBUG_Assert (task->stackBarrier == OS_StackBarrierValue);
        }
    }
}


inline static void schedulerUpdateLastTaskMeasures ()
{
    // First iteration of a new performance measurement period?
    if (!OS_USAGE_UpdatingLastMeasures (&g_OS->usage))
    {
        // No, keep getting performance measurements from the running task on
        // each context switch.
        return;
    }

    // Process last period accumulated performance measurements for every task
    // defined on every state (running, ready and waiting).
    struct OS_TaskControl *task = g_OS->currentTask;
    if (task)
    {
        const int32_t TaskMemory = OS_USAGE_GetUsedTaskMemory (task);
        OS_USAGE_UpdateLastMeasures (&g_OS->usage, &task->usageCpu,
                                        &task->usageMemory, TaskMemory,
                                        task->size);
    }

    for (int i = OS_TaskPriority__BEGIN; i < OS_TaskPriority__COUNT; ++i)
    {
        for (task = (struct OS_TaskControl *) g_OS->tasksWaiting[i].head; task;
             task = (struct OS_TaskControl *) task->node.next)
        {
            const int32_t TaskMemory = OS_USAGE_GetUsedTaskMemory (task);
            OS_USAGE_UpdateLastMeasures (&g_OS->usage, &task->usageCpu,
                                            &task->usageMemory, TaskMemory,
                                            task->size);
        }

        for (task = (struct OS_TaskControl *) g_OS->tasksReady[i].head; task;
             task = (struct OS_TaskControl *) task->node.next)
        {
            const int32_t TaskMemory = OS_USAGE_GetUsedTaskMemory (task);
            OS_USAGE_UpdateLastMeasures (&g_OS->usage, &task->usageCpu,
                                            &task->usageMemory, TaskMemory,
                                            task->size);
        }
    }
}


inline static void schedulerUpdateOwnMeasures ()
{
    // -Approximate- number of cycles used for task scheduling. It depends on
    // 1) External interrupts preemting PendSV (*).
    // 2) Code not taken into account after measurements took place
    //    (ie DWT->CYCCNT).
    // (*) Note that tasks can be preempted too.
    OS_USAGE_UpdateCurrentMeasures (&g_OS->usage, &g_OS->usageCpu, NULL,
                                   DWT->CYCCNT, 0);

    if (OS_USAGE_UpdatingLastMeasures (&g_OS->usage))
    {
        OS_USAGE_UpdateLastMeasures (&g_OS->usage, &g_OS->usageCpu, NULL, 0, 0);
    }
}


inline static void schedulerLastTaskUpdate (const uint32_t CurrentSp,
                                            const uint32_t TaskCycles,
                                            const uint32_t Now)
{
    struct OS_TaskControl *task = g_OS->currentTask;
    if (!task)
    {
        // if g_OS->currentTask == NULL, it is assumed that CurrentSp belongs to
        // the first context switch from MSP or an already terminated task,
        // therefore there is no task control register to update.
        return;
    }

    // Task switch from a previous running task.
    DEBUG_Assert (task->state         == OS_TaskState_Running);
    DEBUG_Assert (task->stackBarrier  == OS_StackBarrierValue);

    // Store task last status.
    task->runCycles += TaskCycles;
    task->sp         = CurrentSp;

    // Memory usage metrics always includes static use of own control structure
    // (sizeof(OS_TaskControl)).
    const int32_t CurMemory = OS_USAGE_GetUsedTaskMemory (task);

    OS_USAGE_UpdateCurrentMeasures (&g_OS->usage, &task->usageCpu,
                                   &task->usageMemory, TaskCycles, CurMemory);

    taskUpdateState (task, Now);

    switch (task->state)
    {
        case OS_TaskState_Ready:
            QUEUE_PushNode (&g_OS->tasksReady[task->priority],
                                            (struct QUEUE_Node *) task);
            break;

        case OS_TaskState_Waiting:
            QUEUE_PushNode (&g_OS->tasksWaiting[task->priority],
                                            (struct QUEUE_Node *) task);
            break;

        default:
            // Invalid current task state for g_OS->currentTask.
            DEBUG_Assert (false);
            break;
    }

    DEBUG_Assert (task->stackBarrier == OS_StackBarrierValue);

    g_OS->currentTask = NULL;
}


inline static void schedulerFindNextTask ()
{
    // There must be no task selected at this point
    DEBUG_Assert (!g_OS->currentTask);

    // Find next task according to priority and a round-robin scheme between
    // tasks with the same priority.
    for (int i = OS_TaskPriority__BEGIN; i < OS_TaskPriority__COUNT; ++i)
    {
        struct QUEUE *queue = &g_OS->tasksReady[i];
        if (queue->head)
        {
            g_OS->currentTask = (struct OS_TaskControl *) queue->head;
            QUEUE_DetachNode (queue, queue->head);
            break;
        }
    }
}


inline static void schedulerSetCurrentTaskReadyToRun (const uint32_t Now)
{
    // At least one task must have been selected.
    DEBUG_Assert (g_OS->currentTask);

    // Last check for stack bounds integrity.
    DEBUG_Assert (g_OS->currentTask->stackBarrier == OS_StackBarrierValue);

    g_OS->currentTask->state = OS_TaskState_Running;

    // First time running?
    if (g_OS->currentTask->startedAt == OS_TicksUndefined)
    {
        g_OS->currentTask->startedAt = Now;
    }

    // Privilege level for Boot and Kernel priorities. Unprivileged for User.
    switch (g_OS->currentTask->priority)
    {
        case OS_TaskPriority_Boot:
        case OS_TaskPriority_Kernel0:
        case OS_TaskPriority_Kernel1:
        case OS_TaskPriority_Kernel2:
            // CONTROL[0] = 0, Privileged in thread mode
            __set_CONTROL ((__get_CONTROL() & ~0b01));
            __ISB ();
            break;

        default:
            // CONTROL[0] = 1, User state in thread mode
            __set_CONTROL ((__get_CONTROL() | 0b01));
            __ISB ();
            break;
    }
}


uint32_t OS_Scheduler (const uint32_t CurrentSp, const uint32_t TaskCycles)
{
    DEBUG_Assert (g_OS);

    g_OS_SchedulerCallPending = 0;

    // if terminatedAt has a valid amount of ticks, scheduler will return a
    // special return value (0, closing) to the PendSV handler code.
    if (g_OS->terminatedAt != OS_TicksUndefined)
    {
        return 0;
    }

    // Avoid getting different tick readings along the scheduling process
    // (May vary depending on external interrupts preempting PendSV).
    const OS_Ticks Now = OS_GetTicks ();

    // First scheduler run
    if (!CurrentSp)
    {
        DEBUG_Assert (g_OS->startedAt == OS_TicksUndefined);
        DEBUG_Assert (!g_OS->currentTask);

        g_OS->startedAt = Now;
    }

    // Update all tasks currently in WAITING state.
    schedulerUpdateWaitingTasks (Now);

    // Update last executed task, if there is one.
    schedulerLastTaskUpdate (CurrentSp, TaskCycles, Now);

    // Find next among all READY tasks.
    schedulerFindNextTask ();

    // Set the selected task ready to be run.
    schedulerSetCurrentTaskReadyToRun (Now);

    // Check if current usage measurement period has ended.
    OS_USAGE_UpdateTarget (&g_OS->usage, Now);

    // Update tasks and scheduler measurements accordingly.
    schedulerUpdateLastTaskMeasures ();
    schedulerUpdateOwnMeasures ();

    // Clear tick barrier and set PendSV to pending again if needed.
    OS_SchedulerTickBarrier__CHECK ();

    return g_OS->currentTask->sp;
}


enum OS_Result OS_SchedulerCallPending ()
{
    if (!OS_RuntimePrivileged ())
    {
        return OS_Result_InvalidCaller;
    }

    // #warning analizar si poner el barrier aca y que sea un IntBarrier.
    // Caso: interrupcion no systick dentro de un bloque Barrier_ACTIVATE()
    //       se activaria PendSV aunque haya alguna tarea que pidio el barrier!
    //       cuando ese barrier se suelta otra vez se actuva PendSV!

    // Tambien analizar si los barriers son realmente necesarios! o sea, esta
    // bueno deferrear un pendsv si alguna tarea va a requerir otro llamado
    // unas instrucciones despues. Pero es dificil de sincronizar.

    // y ademas.. una tarea puede tener el poder de deferrear al scheduler??

    g_OS_SchedulerCallPending = 1;

    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;

    __DSB ();
    __ISB ();

    return OS_Result_OK;
}


void OS_SchedulerTickCallback (OS_Ticks ticks)
{
    // Don't set PendSV to pending if barrier is enabled.
    if (!g_OS_SchedulerTickBarrier)
    {
        OS_SchedulerCallPending ();
    }
    else
    {
        // ... but do record the number of missed attempts.
        ++ g_OS_SchedulerTicksMissed;
    }
}


enum OS_Result OS_SchedulerTickBarrier__ACTIVATE ()
{
    if (!OS_RuntimePrivileged ())
    {
        return OS_Result_InvalidCaller;
    }

    g_OS_SchedulerTickBarrier = 1;

    return OS_Result_OK;
}


enum OS_Result OS_SchedulerTickBarrier__CHECK ()
{
    if (!OS_RuntimePrivileged ())
    {
        return OS_Result_InvalidCaller;
    }

    g_OS_SchedulerTickBarrier = 0;

    if (g_OS_SchedulerTicksMissed)
    {
        g_OS_SchedulerTicksMissed = 0;
        OS_SchedulerCallPending ();
    }

    return OS_Result_OK;
}


enum OS_Result OS_SchedulerTickBarrier__CLEAR ()
{
    if (!OS_RuntimePrivileged ())
    {
        return OS_Result_InvalidCaller;
    }

    g_OS_SchedulerTicksMissed = 0;
    g_OS_SchedulerTickBarrier = 0;

    return OS_Result_OK;
}
