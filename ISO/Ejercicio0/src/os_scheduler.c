/*
    Copyright 2019 Santiago Germino (royconejo@gmail.com)

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
#include "os_scheduler.h"
#include "os_private.h"
#include "os_runtime.h"
#include "debug.h"
#include "chip.h"       // CMSYS


inline static void taskSigWaitEnd (struct OS_TaskControl *tc,
                                   enum OS_Result result)
{
    tc->sigWaitAction = NULL;
    tc->sigWaitObject = NULL;
    tc->sigWaitResult = result;
}


static void taskUpdateState (struct OS_TaskControl *tc, OS_Ticks now)
{
    // "Suspended until" still in the future.
    if (tc->suspendedUntil > now)
    {
        // No signal, keep waiting.
        if (!tc->sigWaitAction)
        {
            tc->state = OS_TaskState_Waiting;
            return;
        }

        DEBUG_Assert (tc->sigWaitObject);

        // Trick to be able to get the correct OS_TaskSelf () in signal action.
        struct OS_TaskControl *ctb = g_OS->currentTask;
        g_OS->currentTask = tc;

        // if the signal the task was waiting for can be adquired then switch
        // it to a ready state.
        if (tc->sigWaitAction (tc->sigWaitObject))
        {
            taskSigWaitEnd (tc, OS_Result_OK);
            tc->suspendedUntil = 0;
        }

        g_OS->currentTask = ctb;
    }
    // There is suspended time but it's no longer in the future.
    // (tc->suspendedUntil > 0 && tc->suspendedUntil <= now)
    else if (tc->suspendedUntil)
    {
        if (!tc->sigWaitAction)
        {
            tc->lastSuspension = tc->suspendedUntil;
        }
        else
        {
            taskSigWaitEnd (tc, OS_Result_Timeout);
        }

        tc->suspendedUntil = 0;
    }

    tc->state = OS_TaskState_Ready;
}


inline static void perfCheckMeasuredPeriod (OS_Ticks now)
{
    if (g_OS->perfTargetTicksNext >= now)
    {
        g_OS->perfTargetTicksNext   = now + g_OS->perfTargetTicksCount;
        g_OS->perfNewMeasureBegins  = true;
    }
    else
    {
        g_OS->perfNewMeasureBegins  = false;
    }
}


inline static void perfCloseLastMeasuredPeriod (struct OS_PerfData *pd)
{
    pd->lastUsage       = pd->curCycles * g_OS->perfCyclesPerTargetTicks;
    pd->lastCycles      = pd->curCycles;
    pd->lastSwitches    = pd->curSwitches;
    pd->curCycles       = 0;
    pd->curSwitches     = 0;
}


inline static void perfUpdateMeasures (struct OS_PerfData *pd,
                                       OS_Cycles taskCycles)
{
    pd->curCycles += taskCycles;
    ++ pd->curSwitches;
}


inline static void schedulerLastTaskUpdate (uint32_t currentSp,
                                            uint32_t taskCycles,
                                            uint32_t now)
{
    struct OS_TaskControl *ct = g_OS->currentTask;
    if (!ct)
    {
        // if g_OS->currentTask == NULL, it is assumed that currentSp
        // belongs to the first context or an already terminated task and
        // therefore there is no corresponding task control register to
        // store it.
        return;
    }

    DEBUG_Assert (ct->state         == OS_TaskState_Running);
    DEBUG_Assert (ct->stackBarrier  == OS_StackBarrierValue);

    // Task switching from a previous running task
    ct->runCycles  += taskCycles;
    ct->sp          = currentSp;

    perfUpdateMeasures (&ct->performance, taskCycles);

    if (g_OS->perfNewMeasureBegins)
    {
        perfCloseLastMeasuredPeriod (&ct->performance);
    }

    taskUpdateState (ct, now);

    switch (ct->state)
    {
        case OS_TaskState_Ready:
            QUEUE_PushNode (&g_OS->tasksReady[ct->priority],
                                            (struct QUEUE_Node *) ct);
            break;

        case OS_TaskState_Waiting:
            QUEUE_PushNode (&g_OS->tasksWaiting[ct->priority],
                                            (struct QUEUE_Node *) ct);
            break;

        default:
            // Invalid current task state for g_OS->currentTask.
            DEBUG_Assert (false);
            break;
    }

    DEBUG_Assert (ct->stackBarrier == OS_StackBarrierValue);

    g_OS->currentTask = NULL;
}


inline static void schedulerSetCurrentTaskToRun (uint32_t now)
{
    // At least the idle task must have been selected.
    DEBUG_Assert (g_OS->currentTask);

    // Last check for stack bounds integrity
    DEBUG_Assert (g_OS->currentTask->stackBarrier == OS_StackBarrierValue);

    g_OS->currentTask->state = OS_TaskState_Running;

    // First time running?
    if (g_OS->currentTask->startedAt == OS_UndefinedTicks)
    {
        g_OS->currentTask->startedAt = now;
    }

    switch (g_OS->currentTask->priority)
    {
        case OS_TaskPriority_Boot:
        case OS_TaskPriority_Drv0:
        case OS_TaskPriority_Drv1:
        case OS_TaskPriority_Drv2:
            // CONTROL[0] = 0, Privileged in thread mode
            __set_CONTROL ((__get_CONTROL() & ~0b01));
            break;

        default:
            // CONTROL[0] = 1, User state in thread mode
            __set_CONTROL ((__get_CONTROL() | 0b01));
            break;
    }
}


uint32_t OS_Scheduler (uint32_t currentSp, uint32_t taskCycles)
{
    DEBUG_Assert (g_OS);

    if (g_OS->terminatedAt != OS_UndefinedTicks)
    {
        return 0;
    }

    const OS_Ticks Now = OS_GetTicks ();

    // First scheduler run
    if (!currentSp)
    {
        DEBUG_Assert (g_OS->startedAt == OS_UndefinedTicks);
        DEBUG_Assert (!g_OS->currentTask);

        g_OS->startedAt             = Now;
        g_OS->perfTargetTicksNext   = Now + g_OS->perfTargetTicksCount;
    }
    else
    {
        perfCheckMeasuredPeriod (Now);
    }

    // Updating tasks in waiting state.
    for (int i = OS_TaskPriority__BEGIN; i < OS_TaskPriority__COUNT; ++i)
    {
        struct QUEUE            *queue  = &g_OS->tasksWaiting[i];
        struct OS_TaskControl   *task   = (struct OS_TaskControl *) queue->head;

        while (task)
        {
            DEBUG_Assert (task->stackBarrier == OS_StackBarrierValue);

            taskUpdateState (task, Now);

            if (task->state == OS_TaskState_Ready)
            {
                // Timeout reached. A ready task will be moved to the
                // corresponding task list.
                QUEUE_DetachNode (queue, (struct QUEUE_Node *) task);
                QUEUE_PushNode   (&g_OS->tasksReady[i], (struct QUEUE_Node *)
                                  task);
            }
            else if (g_OS->perfNewMeasureBegins)
            {
                // TaskState_Ready tasks will be checked in the scheduling loop
                // below.
                perfCloseLastMeasuredPeriod (&task->performance);
            }

            DEBUG_Assert (task->stackBarrier == OS_StackBarrierValue);

            task = (struct OS_TaskControl *) task->node.next;
        }
    }

    // This is done at the first iteration of a new performance measurement
    // period to update performance metrics for all ready tasks.
    if (g_OS->perfNewMeasureBegins)
    {
        for (int i = OS_TaskPriority__BEGIN; i < OS_TaskPriority__COUNT; ++i)
        {
            struct QUEUE            *queue  = &g_OS->tasksReady[i];
            struct OS_TaskControl   *task   = (struct OS_TaskControl *)
                                                queue->head;
            while (task)
            {
                perfCloseLastMeasuredPeriod (&task->performance);
                task = (struct OS_TaskControl *) task->node.next;
            }
        }
    }

    // Update the last executed task, if there is one.
    schedulerLastTaskUpdate (currentSp, taskCycles, Now);

    // There must be no task selected at this point
    DEBUG_Assert (!g_OS->currentTask);

    // Find the next task according to priority on a round-robin scheme.
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

    // Set the selected task ready to run
    schedulerSetCurrentTaskToRun (Now);

    OS_SchedulerTickBarrier__CHECK ();

    // Aprox. number of cycles used for task scheduling.
    g_OS->schedulerRunCycles += DWT->CYCCNT;

    return g_OS->currentTask->sp;
}


// Must be in privileged mode to execute this code
void OS_SchedulerWakeup ()
{
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    __DSB ();
    __ISB ();
}


void OS_SchedulerTickCallback (OS_Ticks ticks)
{
    // This function is always called in handler mode
    // Barrier enabled or SysTick preemted a PendSV interrupt in progress?
    # warning agregar pendsv en progreso
    if (!g_OS_SchedulerTickBarrier )
    {
        OS_SchedulerWakeup ();
    }
    else
    {
        ++ g_OS_SchedulerTicksMissed;
    }
}


enum OS_Result OS_SchedulerTickBarrier__ACTIVATE ()
{
    if (!OS_RuntimePrivileged ())
    {
        return OS_Result_InvalidCall;
    }

    g_OS_SchedulerTickBarrier = 1;

    return OS_Result_OK;
}


enum OS_Result OS_SchedulerTickBarrier__CHECK ()
{
    if (!OS_RuntimePrivileged ())
    {
        return OS_Result_InvalidCall;
    }

    g_OS_SchedulerTickBarrier = 0;

    if (g_OS_SchedulerTicksMissed)
    {
        g_OS_SchedulerTicksMissed = 0;
        OS_SchedulerWakeup ();
    }

    return OS_Result_OK;
}


enum OS_Result OS_SchedulerTickBarrier__CLEAR ()
{
    if (!OS_RuntimePrivileged ())
    {
        return OS_Result_InvalidCall;
    }

    g_OS_SchedulerTicksMissed = 0;
    g_OS_SchedulerTickBarrier = 0;

    return OS_Result_OK;
}
