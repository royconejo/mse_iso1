/*
    Copyright 2019 Santiago Germino (royconejo@gmail.com)

    RETRO-CIAA™ Library - Preemtive multitasking Operating System (RETRO-OS).
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
#include "os_critical.h"
#include "queue.h"
#include "semaphore.h"
#include "os_mutex.h"
#include "debug.h"
#include <string.h>
#include "chip.h"   // CMSIS

#ifndef RETROCIAA_OS_CUSTOM_SYSTICK
#include "systick.h"
#endif


#define OS_StackBarrierValue        0xDEADBEEF
#define OS_IntegerRegisters         17
#define OS_FPointRegisters          (16 + 1 + 16)
#define OS_ContextRegisters         (OS_FPointRegisters + OS_IntegerRegisters)
// NOTE: this number must be big enough for any task to be able to execute the
//       scheduler!
#define OS_MinAppStackSize          128
#define OS_TaskMinBufferSize        (sizeof(struct OS_TaskControl) \
                                            + (OS_ContextRegisters * 4) \
                                            + OS_MinAppStackSize)
#define OS_UndefinedTicks           ((OS_Ticks) -1)
#define OS_PerfTargetTicks        1000


typedef uint64_t    OS_Cycles;
typedef bool        (*OS_SigAction) (void *sig);


static struct OS    *g_OS                   = NULL;
static const char   *TaskNoDescription      = "N/A";
static const char   *TaskIdleDescription    = "IDLE";


enum OS_TaskState
{
    OS_TaskState_Terminated,
    OS_TaskState_Waiting,
    OS_TaskState_Ready,
    OS_TaskState_Running
};


struct OS_PerfData
{
    OS_Cycles               curCycles;
    uint32_t                curSwitches;
    OS_Cycles               lastCycles;
    uint32_t                lastSwitches;
    float                   lastUsage;
};


struct OS_TaskControl
{
    struct QUEUE_Node       node;
    uint32_t                size;
    const char              *description;
    uint32_t                retValue;
    OS_Ticks                startedAt;
    OS_Ticks                terminatedAt;
    OS_Ticks                suspendedUntil;
    OS_Ticks                lastSuspension;
    OS_SigAction            sigWaitAction;
    void                    *sigWaitObject;
    enum OS_Result          sigWaitResult;
    OS_Cycles               runCycles;
    struct OS_PerfData      performance;
    enum OS_TaskPriority    priority;
    enum OS_TaskState       state;
    uint32_t                sp;
    uint32_t                stackBarrier;
};


struct OS
{
    uint32_t                mainSp;
    OS_Ticks                startedAt;
    OS_Ticks                terminatedAt;
    bool                    startedForever;
    OS_Cycles               schedulerRunCycles;
    uint32_t                perfTargetTicks;
    uint32_t                perfTargetTicksCount;
    float                   perfCyclesPerTargetTicks;
    bool                    perfNewMeasureBegins;
    struct OS_PerfData      performance;
   // The only one task in the RUNNING state
   struct OS_TaskControl    *currentTask;
   // Tasks in READY state
   struct QUEUE             tasksReady      [OS_TaskPriorityLevels];
   // Tasks in any other state
   struct QUEUE             tasksWaiting    [OS_TaskPriorityLevels];
   // Idle task buffer (this task is internal and belongs to the OS)
   uint8_t                  idleTaskBuffer  [OS_TaskMinBufferSize];
};


static OS_TaskRet taskIdle (OS_TaskParam arg)
{
	while (1)
    {
        __WFI ();
    }

    return 0;
}


static void schedulerTick (OS_Ticks ticks)
{
    OS_SchedulerWakeup ();
}


static void startOs (bool forever)
{
    DEBUG_Assert (g_OS);

    g_OS->startedForever = forever;

    OS_SetTickHook (schedulerTick);
}


static void terminateOs ()
{
    DEBUG_Assert (g_OS && !g_OS->startedForever);

    OS_SetTickHook (NULL);
    g_OS = NULL;
}


static void taskYield ()
{
    OS_SchedulerWakeup ();
    // TODO: ver si este wfi es necesario
    __WFI ();
}


// when a task returns from its main function it is assumed that taskControl
// is NULL (R1 == 0).
static void taskTerminate (uint32_t retValue,
                           struct OS_TaskControl *taskControl)
{
    OS_CriticalSection__ENTER ();

    // Tasks can be abruptly terminated calling OS_TaskTerminate(TaskBuffer)
    // from another task.

    // Own task returning from its main function or by calling
    // OS_TaskTerminate(NULL, x).
    if (!taskControl)
    {
        DEBUG_Assert (g_OS->currentTask->state == OS_TaskState_Running);

        taskControl = g_OS->currentTask;
    }
    else if (taskControl->state == OS_TaskState_Terminated)
    {
        // Trying to termine an already terminated task, do nothing.
        return;
    }

    switch (taskControl->state)
    {
        case OS_TaskState_Running:
            g_OS->currentTask = NULL;
            break;

        case OS_TaskState_Ready:
            QUEUE_DetachNode (&g_OS->tasksReady[taskControl->priority],
                                            (struct QUEUE_Node *) taskControl);
            break;

        default:
            QUEUE_DetachNode (&g_OS->tasksWaiting[taskControl->priority],
                                            (struct QUEUE_Node *) taskControl);
            break;
    }

    taskControl->retValue       = retValue;
    taskControl->state          = OS_TaskState_Terminated;
    taskControl->terminatedAt   = OS_GetTicks ();

    // There is no running task in the scheduler.
    if (!g_OS->currentTask)
    {
        OS_CriticalSection__CLEAR ();
        OS_SchedulerWakeup ();
        // If closed from the same task (task auto-termination) the stack
        // pointer is no longer used. Context gets trapped in an infinite loop
        // for debug purposes only.
        while (1)
        {
            __WFI ();
        }
    }

    OS_CriticalSection__LEAVE ();
}


static void taskStackInit (struct OS_TaskControl *taskControl, OS_Task task,
                           void *taskParam)
{
    uint32_t *stackTop = &((uint32_t *)taskControl) [(taskControl->size >> 2)];

    *(--stackTop) = 1 << 24;                  // xPSR.T = 1
	*(--stackTop) = (uint32_t) task;          // xPC
	*(--stackTop) = (uint32_t) taskTerminate; // xLR
    *(--stackTop) = 0;                        // R12
    *(--stackTop) = 0;                        // R3
    *(--stackTop) = 0;                        // R2
    *(--stackTop) = 0;                        // R1
	*(--stackTop) = (uint32_t) taskParam;     // R0
    // LR stacked at interrupt handler
	*(--stackTop) = 0xFFFFFFF9;               // LR IRQ
    // R4-R11 stacked at interrupt handler
    *(--stackTop) = 0;                        // R11
    *(--stackTop) = 0;                        // R10
    *(--stackTop) = 0;                        // R9
    *(--stackTop) = 0;                        // R8
    *(--stackTop) = 0;                        // R7
    *(--stackTop) = 0;                        // R6
    *(--stackTop) = 0;                        // R5
    *(--stackTop) = 0;                        // R4

  	taskControl->sp = (uint32_t) stackTop;
}


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


inline static enum OS_Result taskDelayFrom__BEGIN ()
{
    if (!g_OS)
    {
        return OS_Result_NotInitialized;
    }

    OS_CriticalSection__ENTER ();

    if (!g_OS->currentTask)
    {
        OS_CriticalSection__LEAVE ();
        return OS_Result_NoCurrentTask;
    }

    return OS_Result_OK;
}


inline static enum OS_Result taskDelayFrom__END (OS_Ticks ticks, OS_Ticks from)
{
    g_OS->currentTask->suspendedUntil = from + ticks;

    OS_CriticalSection__CLEAR ();
    taskYield ();

    return OS_Result_OK;
}


inline static void perfCheckMeasuredPeriod ()
{
    if (g_OS->perfTargetTicksCount >= g_OS->perfTargetTicks)
    {
        g_OS->perfTargetTicksCount  = 0;
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


uint32_t OS_GetNextStackPointer (uint32_t currentSp, uint32_t taskCycles)
{
    DWT->CYCCNT = 0;

    __CLREX ();

    DEBUG_Assert (g_OS);

    if (g_OS->terminatedAt != OS_UndefinedTicks)
    {
        return g_OS->mainSp;
    }

    const OS_Ticks Now = OS_GetTicks ();

    OS_CriticalSection__ENTER ();

    OS_ResetSchedulingMisses ();

    perfCheckMeasuredPeriod ();

    if (g_OS->startedAt == OS_UndefinedTicks)
    {
        g_OS->mainSp    = currentSp;
        g_OS->startedAt = Now;
    }

    // Updating task in waiting state.
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

    // This is done on every new performance measurement peoriod to update
    // the metrics of all ready tasks.
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

    // Updating the last executed task, if there is one.
    {
        struct OS_TaskControl *ct = g_OS->currentTask;
        if (ct)
        {
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

            taskUpdateState (ct, Now);

            switch (ct->state)
            {
                case OS_TaskState_Waiting:
                    QUEUE_PushNode (&g_OS->tasksWaiting[ct->priority],
                                            (struct QUEUE_Node *) ct);
                    break;

                case OS_TaskState_Ready:
                    QUEUE_PushNode (&g_OS->tasksReady[ct->priority],
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
        else
        {
            // if g_OS->currentTask == NULL, it is assumed that currentSp
            // belongs to the first context or an already terminated task and
            // therefore there is no corresponding task control register to
            // store it.
        }
    }

    DEBUG_Assert (!g_OS->currentTask);

    // Find the next task according to priority and round-robin order.
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

    // At least the idle task must have been selected.
    DEBUG_Assert (g_OS->currentTask);

    // Last check for stack bounds integrity
    DEBUG_Assert (g_OS->currentTask->stackBarrier == OS_StackBarrierValue);

    g_OS->currentTask->state = OS_TaskState_Running;

    // First time running?
    if (g_OS->currentTask->startedAt == OS_UndefinedTicks)
    {
        g_OS->currentTask->startedAt = Now;
    }

    ++ g_OS->perfTargetTicksCount;

    // Aprox. number of cycles used for scheduling tasks.
    g_OS->schedulerRunCycles += DWT->CYCCNT;

    OS_CriticalSection__CLEAR ();

    return g_OS->currentTask->sp;
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

    // PendSV with minimum priority
    NVIC_SetPriority (PendSV_IRQn, (1 << __NVIC_PRIO_BITS) - 1);

    // Enable MCU cycle counter
    DWT->CTRL |= 1 << DWT_CTRL_CYCCNTENA_Pos;

    for (int i = OS_TaskPriority__BEGIN; i < OS_TaskPriority__COUNT; ++i)
    {
        QUEUE_Init (&os->tasksReady[i]);
        QUEUE_Init (&os->tasksWaiting[i]);
    }

    os->startedAt                   = OS_UndefinedTicks;
    os->terminatedAt                = OS_UndefinedTicks;
    os->perfTargetTicks             = OS_PerfTargetTicks;
    // NOTE: a Tick rate of 1/1000 of a second is assumed!
    // 1000 - 100 (percent) = 10
    os->perfCyclesPerTargetTicks    = 1.0f / ((float) SystemCoreClock
                                                / (float) os->perfTargetTicks
                                                * 10.0f);
    OS_CriticalSection__RESET   ();
    OS_ResetSchedulingMisses    ();

    g_OS = os;

    // Idle task with lowest priority level (one point below lower normal task
    // priority).
    OS_TaskStart (os->idleTaskBuffer, sizeof(os->idleTaskBuffer),
                  taskIdle, NULL,
                  OS_TaskPriorityIdle, TaskIdleDescription);

    return OS_Result_OK;
}


void OS_Forever ()
{
    startOs (true);

    while (1)
    {
        __WFI ();
    }
}


void OS_Start ()
{
    startOs (false);

    // First call to pendsv will do a context switch to the first available task
    // defined in the OS (taskIdle if the user had not registered any task).
    __WFI ();

    // A context switch entering this point means the OS has been instructed to
    // terminate.
    OS_CriticalSection__ENTER ();

    terminateOs ();

    OS_CriticalSection__RESET ();
}


bool OS_Terminate ()
{
    if (!g_OS)
    {
        // Nothing to terminate
        return true;
    }

    if (g_OS->startedForever)
    {
        // OS cannot be terminated
        return false;
    }

    OS_CriticalSection__ENTER ();

    g_OS->terminatedAt = OS_GetTicks ();

    OS_CriticalSection__LEAVE ();

    // When terminatedAt is given a valid number of ticks, there will be a
    // context switch to resume execution at mainSp (which was the first context
    // switched from in pendsv). After that, scheduler execution will be
    // terminated along with any defined task.
    while (1)
    {
        __WFI ();
    }
}


enum OS_Result OS_TaskStart (void *taskBuffer, uint32_t taskBufferSize,
                             OS_Task task, void *taskParam,
                             enum OS_TaskPriority priority,
                             const char *description)
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

    if (!description)
    {
        description = TaskNoDescription;
    }

    memset (taskBuffer, 0, taskBufferSize);

    struct OS_TaskControl *taskControl = (struct OS_TaskControl *) taskBuffer;

    taskControl->size           = taskBufferSize;
    taskControl->description    = description;
    taskControl->startedAt      = OS_UndefinedTicks;
    taskControl->terminatedAt   = OS_UndefinedTicks;
    taskControl->priority       = priority;
    taskControl->state          = OS_TaskState_Ready;    
    taskControl->stackBarrier   = OS_StackBarrierValue;

    taskStackInit (taskControl, task, taskParam);

    OS_CriticalSection__ENTER ();

    QUEUE_PushNode  (&g_OS->tasksReady[taskControl->priority],
                                            (struct QUEUE_Node *) taskControl);
    OS_CriticalSection__LEAVE ();

    DEBUG_Assert (taskControl->stackBarrier == OS_StackBarrierValue);

    return OS_Result_OK;
}


enum OS_Result OS_TaskTerminate (void *taskBuffer, OS_TaskRet retVal)
{
    if (!g_OS)
    {
        return OS_Result_NotInitialized;
    }

    taskTerminate (retVal, (struct OS_TaskControl *) taskBuffer);

    return OS_Result_OK;
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
    if (!g_OS)
    {
        return OS_Result_NotInitialized;
    }

    if (!g_OS->currentTask)
    {
        return OS_Result_NoCurrentTask;
    }

    taskYield ();
    return OS_Result_OK;
}


enum OS_Result OS_TaskPeriodicDelay (OS_Ticks ticks)
{
    const enum OS_Result Result = taskDelayFrom__BEGIN ();
    return (Result != OS_Result_OK)? Result
                : taskDelayFrom__END (g_OS->currentTask->lastSuspension, ticks);
}


enum OS_Result OS_TaskDelayFrom (OS_Ticks ticks, OS_Ticks from)
{
    const enum OS_Result Result = taskDelayFrom__BEGIN ();
    return (Result != OS_Result_OK)? Result
                : taskDelayFrom__END (from, ticks);
}


enum OS_Result OS_TaskDelay (OS_Ticks ticks)
{
    return OS_TaskDelayFrom (ticks, OS_GetTicks());
}


enum OS_Result OS_TaskWaitForSignal (enum OS_TaskSignalType sigType,
                                     void *sigObject, OS_Ticks timeout)
{
    if (!g_OS)
    {
        return OS_Result_NotInitialized;
    }

    if (!sigObject || sigType >= OS_TaskSignalType__COUNT)
    {
        return OS_Result_InvalidParams;
    }

    OS_CriticalSection__ENTER ();

    if (!g_OS->currentTask)
    {
        OS_CriticalSection__LEAVE ();
        return OS_Result_NoCurrentTask;
    }

    OS_SigAction sigAction = NULL;
    switch (sigType)
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

    const bool ActionResult = sigAction (sigObject);

    if (ActionResult || !timeout)
    {
        OS_CriticalSection__LEAVE ();
        return ActionResult? OS_Result_OK : OS_Result_Timeout;
    }

    g_OS->currentTask->sigWaitAction    = sigAction;
    g_OS->currentTask->sigWaitObject    = sigObject;
    g_OS->currentTask->sigWaitResult    = OS_Result_NotInitialized;
    g_OS->currentTask->suspendedUntil   = OS_GetTicks() + timeout;

    OS_CriticalSection__CLEAR ();

    taskYield ();

    return g_OS->currentTask->sigWaitResult;
}


enum OS_Result OS_TaskReturnValue (void *taskBuffer, uint32_t *retValue)
{
    if (!taskBuffer || !retValue)
    {
        return OS_Result_InvalidParams;
    }

    struct OS_TaskControl *taskControl = (struct OS_TaskControl *) taskBuffer;

    if (taskControl->state != OS_TaskState_Terminated)
    {
        return OS_Result_InvalidState;
    }

    *retValue = taskControl->retValue;

    return OS_Result_OK;
}

