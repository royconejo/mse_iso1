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
#include "os.h"
#include "debug.h"
#include <string.h>
#include "chip.h"   // CMSIS

#ifndef RETROCIAA_OS_STANDALONE
#include "systick.h"
#endif


static struct OS *g_OS = NULL;


static void schedulerWakeup ()
{
    __ISB ();
    __DSB ();

    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}


static void schedulerTick (OS_Ticks ticks)
{
    schedulerWakeup ();
}


static OS_TaskRet taskIdle (OS_TaskParam arg)
{
	while (1)
    {
        __WFI ();
    }

    return 0;
}


static void taskDetach (struct OS_TaskControl *taskControl)
{
    // Detaching task from the chain
    if (taskControl->prev)
    {
        taskControl->prev->next = taskControl->next;
    }

    if (taskControl->next)
    {
        taskControl->next->prev = taskControl->prev;
    }

    taskControl->prev = NULL;
    taskControl->next = NULL;
}


static void taskDetachEnds (struct OS_TaskControlEnds *ends,
                            struct OS_TaskControl *taskControl)
{
    DEBUG_Assert ((ends->firstTask && ends->lastTask)
                    || (!ends->firstTask && !ends->lastTask));

    const bool TaskFirst = (ends->firstTask == taskControl);
    const bool TaskLast  = (ends->lastTask  == taskControl);

    // Removing the only task in the linked list
    if (TaskFirst && TaskLast)
    {
        DEBUG_Assert (!taskControl->prev && !taskControl->next);
        ends->firstTask = NULL;
        ends->lastTask  = NULL;
    }
    // Removing the first task
    else if (TaskFirst)
    {
        DEBUG_Assert (taskControl->next);
        ends->firstTask = taskControl->next;
    }
    // Removing the last task
    else if (TaskLast)
    {
        DEBUG_Assert (taskControl->prev);
        ends->lastTask = taskControl->prev;
    }

    taskDetach (taskControl);
}


static void taskEnd (uint32_t retValue, struct OS_TaskControl *taskControl)
{
    // Task returned from its main function or by calling OS_TaskEnd(NULL)
    if (taskControl->state == OS_TaskState_Running)
    {
        DEBUG_Assert (g_OS->currentTask == taskControl);
        g_OS->currentTask = NULL;
    }
    // Task terminated by OS_TaskEnd(TaskBuffer) called from another task
    else
    {
        DEBUG_Assert (g_OS->currentTask != taskControl);
        if (taskControl->state == OS_TaskState_Ready)
        {
            taskDetachEnds (&g_OS->tasksReady[taskControl->priority],
                            taskControl);
        }
        else
        {
            taskDetachEnds (&g_OS->tasksSuspended[taskControl->priority],
                            taskControl);
        }
    }

    taskControl->retValue           = retValue;
    taskControl->state              = OS_TaskState_Terminated;
    taskControl->terminatedAtTicks  = OS_GetTicks ();

    if (!g_OS->currentTask)
    {
        schedulerWakeup ();
        // If closed from the same task (task auto-termination) the stack
        // pointer is no longer used and its context gets trapped in an infinite
        // loop.
        while (1)
        {
            __WFI ();
        }
    }
}


static void taskStackInit (struct OS_TaskControl *taskControl, OS_Task task,
                           void *taskParam)
{
    uint32_t *stackTop = &((uint32_t *)taskControl)
                                    [(taskControl->size >> 2) - 1];

    *stackTop-- = 1 << 24;                  // xPSR.T = 1
	*stackTop-- = (uint32_t) task;          // xPC
	*stackTop-- = (uint32_t) taskEnd;       // xLR
    *stackTop-- = 0;                        // R12
    *stackTop-- = 0;                        // R3
    *stackTop-- = 0;                        // R2
    *stackTop-- = (uint32_t) taskControl;   // R1
	*stackTop-- = (uint32_t) taskParam;     // R0
    // LR stacked at interrupt handler
	*stackTop-- = 0xFFFFFFF9;               // LR IRQ
    // R4-R11 stacked at interrupt handler
    *stackTop-- = 0;                        // R11
    *stackTop-- = 0;                        // R10
    *stackTop-- = 0;                        // R9
    *stackTop-- = 0;                        // R8
    *stackTop-- = 0;                        // R7
    *stackTop-- = 0;                        // R6
    *stackTop-- = 0;                        // R5
    *stackTop   = 0;                        // R4

    // &((uint32_t *)taskControl)[StackSizeWords - 17]
  	taskControl->sp = (uint32_t) stackTop;
}


static void taskPush (struct OS_TaskControl *lastTc,
                      struct OS_TaskControl *newTc)
{
    if (!lastTc)
    {
        return;
    }

    lastTc->next = newTc;
    newTc->prev  = lastTc;
}


static void taskPushEnds (struct OS_TaskControlEnds *ends,
                          struct OS_TaskControl *tc)
{
    if (!ends->firstTask)
    {
        DEBUG_Assert (!ends->lastTask);
        ends->firstTask = tc;
    }
    else
    {
        DEBUG_Assert (ends->lastTask);
        taskPush (ends->lastTask, tc);
    }

    ends->lastTask = tc;
}


static void taskUpdateState (struct OS_TaskControl *tc, OS_Ticks now)
{
    if (tc->suspendedUntilTicks > now)
    {
        tc->state = OS_TaskState_Suspended;
        return;
    }

    if (tc->suspendedUntilTicks && tc->suspendedUntilTicks <= now)
    {
        tc->lastSuspensionTicks = tc->suspendedUntilTicks;
        tc->suspendedUntilTicks = 0;
    }

    tc->state = OS_TaskState_Ready;
}


uint32_t OS_GetNextStackPointer (uint32_t currentSp, uint32_t taskCycles)
{
    DWT->CYCCNT = 0;

    DEBUG_Assert (g_OS);

    const OS_Ticks Now = OS_GetTicks ();

    // Suspended tasks update
    for (enum OS_TaskPriority i = OS_TaskPriorityLevel0;
         i < OS_TaskPriorityLevels; ++i)
    {
        struct OS_TaskControlEnds   *ends = &g_OS->tasksSuspended[i];
        struct OS_TaskControl       *task = ends->firstTask;

        while (task)
        {
            DEBUG_Assert (task->stackBarrier == OS_StackBarrierValue);

            struct OS_TaskControl *next = task->next;
            taskUpdateState (task, Now);

            if (task->state == OS_TaskState_Ready)
            {
                // Timeout reached, task is ready
                taskDetachEnds  (ends, task);
                taskPushEnds    (&g_OS->tasksReady[i], task);
            }
            task = next;
        }
    }

    // if g_OS->currentTask == NULL, it is assumed that currentSp belongs to the
    // first context or an already terminated task and therefore not saved.
    {
        struct OS_TaskControl *ct = g_OS->currentTask;
        if (ct)
        {
            ct->cycles += taskCycles;
            ct->sp      = currentSp;

            DEBUG_Assert (ct->stackBarrier == OS_StackBarrierValue);

            if (ct->state == OS_TaskState_Running)
            {
                taskUpdateState (ct, Now);

                switch (ct->state)
                {
                    case OS_TaskState_Suspended:
                        taskPushEnds (&g_OS->tasksSuspended[ct->priority], ct);
                        break;

                    case OS_TaskState_Ready:
                        taskPushEnds (&g_OS->tasksReady[ct->priority], ct);
                        break;

                    default:
                        // Invalid current task state for g_OS->currentTask.
                        DEBUG_Assert (false);
                        break;
                }
            }

            DEBUG_Assert (ct->stackBarrier == OS_StackBarrierValue);

            g_OS->currentTask = NULL;
        }
    }

    for (enum OS_TaskPriority i = OS_TaskPriorityLevel0;
         i < OS_TaskPriorityLevels; ++i)
    {
        struct OS_TaskControlEnds *ends = &g_OS->tasksReady[i];
        if (ends->firstTask)
        {
            g_OS->currentTask = ends->firstTask;
            taskDetachEnds (ends, ends->firstTask);
            break;
        }
    }

    // At least the idle task must have been found.
    DEBUG_Assert (g_OS->currentTask);

    // Last check for stack bounds integrity
    DEBUG_Assert (g_OS->currentTask->stackBarrier == OS_StackBarrierValue);

    if (g_OS->startedAtTicks == OS_UndefinedTicks)
    {
        g_OS->startedAtTicks = OS_GetTicks ();
    }

    g_OS->currentTask->state = OS_TaskState_Running;

    // Cycles used for scheduling.
    g_OS->schedulerCycles += DWT->CYCCNT;

    return g_OS->currentTask->sp;
}


enum OS_Result OS_Init (struct OS *o)
{
    if (g_OS)
    {
        return OS_Result_AlreadyInitialized;
    }

    memset (o, 0, sizeof(struct OS));

    // PendSV with minimum priority
    NVIC_SetPriority (PendSV_IRQn, (1 << __NVIC_PRIO_BITS) - 1);

    // Enable MCU cycle counter
    DWT->CTRL |= 1 << DWT_CTRL_CYCCNTENA_Pos;

    o->startedAtTicks = OS_UndefinedTicks;
    g_OS = o;

    // Idle task with lowest priority level (one point below lower normal task
    // priority).
    OS_TaskStart (o->idleTaskBuffer, sizeof(o->idleTaskBuffer), taskIdle, NULL,
                  OS_TaskPriorityIdle);

    return OS_Result_OK;
}


enum OS_Result OS_TaskStart (void *taskBuffer, uint32_t taskBufferSize,
                             OS_Task task, void *taskParam,
                             enum OS_TaskPriority priority)
{
    if (!g_OS)
    {
        return OS_Result_NotInitialized;
    }

    if (!task || priority >= OS_TaskPriorityLevels)
    {
        return OS_Result_InvalidParams;
    }

    // taskBuffer pointer must be aligned on a 4 byte boundary
    if ((uint32_t)taskBuffer & 0b11)
    {
        return OS_Result_InvalidBufferAlignment;
    }

    // taskBufferSize must be enough to run the scheduler and must be multiple
    // of 4.
    if (taskBufferSize < OS_TaskMinBufferSize || taskBufferSize & 0b11)
    {
        return OS_Result_InvalidBufferSize;
    }

    memset (taskBuffer, 0, taskBufferSize);

    struct OS_TaskControl *taskControl = (struct OS_TaskControl *) taskBuffer;

    taskControl->size               = taskBufferSize;
    taskControl->startedAtTicks     = OS_UndefinedTicks;
    taskControl->terminatedAtTicks  = OS_UndefinedTicks;
    taskControl->priority           = priority;
    taskControl->state              = OS_TaskState_Ready;
    taskControl->stackBarrier       = OS_StackBarrierValue;

    taskStackInit   (taskControl, task, taskParam);
    taskPushEnds    (&g_OS->tasksReady[taskControl->priority], taskControl);

    DEBUG_Assert (taskControl->stackBarrier == OS_StackBarrierValue);

    return OS_Result_OK;
}


enum OS_Result OS_TaskEnd (void *taskBuffer, OS_TaskRet retVal)
{
    if (!g_OS)
    {
        return OS_Result_NotInitialized;
    }

    struct OS_TaskControl *taskControl = (struct OS_TaskControl *) taskBuffer;

    if (!taskControl)
    {
        DEBUG_Assert (g_OS->currentTask
                        && g_OS->currentTask->state == OS_TaskState_Running);

        taskControl = g_OS->currentTask;
    }

    taskEnd (retVal, taskControl);

    return OS_Result_OK;
}


enum OS_Result OS_TaskYield ()
{
    if (!g_OS)
    {
        return OS_Result_NotInitialized;
    }

    if (!g_OS->currentTask)
    {
        return OS_Result_NoCurrentTask;
    }

    schedulerWakeup ();
    __WFI ();

    return OS_Result_OK;
}


enum OS_Result OS_TaskPeriodicDelay (OS_Ticks ticks)
{
    if (!g_OS)
    {
        return OS_Result_NotInitialized;
    }

    if (!g_OS->currentTask)
    {
        return OS_Result_NoCurrentTask;
    }

    g_OS->currentTask->suspendedUntilTicks =
                        g_OS->currentTask->lastSuspensionTicks + ticks;

    return OS_TaskYield ();
}


enum OS_Result OS_TaskDelayFrom (OS_Ticks ticks, OS_Ticks from)
{
    if (!g_OS)
    {
        return OS_Result_NotInitialized;
    }

    if (!g_OS->currentTask)
    {
        return OS_Result_NoCurrentTask;
    }

    g_OS->currentTask->suspendedUntilTicks = from + ticks;

    return OS_TaskYield ();
}


enum OS_Result OS_TaskDelay (OS_Ticks ticks)
{
    return OS_TaskDelayFrom (ticks, OS_GetTicks());
}


void OS_Start ()
{
    DEBUG_Assert (g_OS);

    g_OS->startedAtTicks = OS_GetTicks ();
    OS_SetTickHook (schedulerTick);

    while (1)
    {
        __WFI ();
    }
}


#ifdef RETROCIAA_SYSTICK
inline OS_Ticks OS_GetTicks()
{
    return SYSTICK_Now ();
}


inline void OS_SetTickHook (OS_TickHook schedulerTickHook)
{
    SYSTICK_SetHook (schedulerTickHook);
}
#endif
