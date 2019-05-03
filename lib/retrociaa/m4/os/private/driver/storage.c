#include "storage.h"
#include "../opaque.h"
#include "../../scheduler.h"
#include "../../../base/debug.h"
#include <string.h>



inline static struct OS_DRIVER_StorageData * getStorageData (
                                                struct OS_TaskControl *task)
{
    return (struct OS_DRIVER_StorageData *) &((uint8_t *) task)
                            [task->size - sizeof(struct OS_DRIVER_StorageData)];

   // return ((struct OS_DRIVER_StorageData *) &((uint8_t *)task)[task->size]) --;
}


uint32_t OS_DRIVER_StorageBufferSize (struct OS_DRIVER_StorageInitParams
                                      *initParams)
{
    if (!DEBUG_Assert (initParams && initParams->jobs))
    {
        return 0;
    }

    return sizeof(struct OS_DRIVER_StorageJob) * initParams->jobs
                                    + sizeof(struct OS_DRIVER_StorageData);
}


enum OS_Result OS_DRIVER_StorageInit (struct OS_TaskControl *task,
                                      struct OS_DRIVER_StorageInitParams
                                      *initParams)
{
    if (!task || !initParams || !initParams->jobs)
    {
        return OS_Result_InvalidParams;
    }

    task->priority = OS_TaskPriority_DriverStorage;

    const uint32_t  BufferSize  = OS_DRIVER_StorageBufferSize (initParams);
    uint8_t         *buffer     = (uint8_t *) task;

    if (!DEBUG_Assert (BufferSize && !(BufferSize & 0b11)))
    {
        return OS_Result_InvalidBufferSize;
    }

    task->stackTop = task->size - BufferSize;

    if ((uint32_t)&buffer[task->stackTop] & 0b11)
    {
        return OS_Result_InvalidBufferAlignment;
    }

    memset (&buffer[task->stackTop], 0, BufferSize);

    struct OS_DRIVER_StorageData *data = getStorageData (task);

    data->maxJobs     = initParams->jobs;
    data->nextFreeJob = (struct OS_DRIVER_StorageJob *)
                                                    &buffer[task->stackTop];
    return OS_Result_OK;
}


// WARNING: Whoever call this function must assure the integrity of *sa contents
//          in memory across all operations involving that data. This means to
//          avoid destruction of the local struct after exiting the OS API call
//          that produced the syscall. This is currently managed by putting the
//          caller task to sleep until the storage operation completes, but can
//          be made asynchronic by performing a deep copy of the data.
enum OS_Result OS_DRIVER_StorageJobAdd (struct OS_TaskControl *task,
                                        struct OS_TaskDriverStorageAccess *sa)
{
    if (!task || !sa)
    {
        return OS_Result_InvalidParams;
    }

    struct OS_DRIVER_StorageData *data = getStorageData (task);

    if (data->queue.elements >= data->maxJobs)
    {
        return OS_Result_BufferFull;
    }

    struct OS_DRIVER_StorageJob *job = data->nextFreeJob;
    // This should never happen given the number of elements in the queue is
    // less than data->maxJobs.
    if (!DEBUG_Assert (!job->access))
    {
        return OS_Result_AssertionFailed;
    }

    // Next free job. This works because jobs are consumed in a FIFO scheme.
    if ((void *)(++ data->nextFreeJob) == (void *)data)
    {
        data->nextFreeJob = (struct OS_DRIVER_StorageJob *)
                                            &((uint8_t *)task)[task->stackTop];
    }

    job->access = sa;

    if (!QUEUE_PushNode (&data->queue, (struct QUEUE_Node *) job))
    {
        return OS_Result_Error;
    }

    return OS_Result_OK;
}


enum OS_Result OS_DRIVER_StorageJobTake (struct OS_TaskControl *task,
                                         struct OS_TaskDriverStorageAccess **sa)
{
    if (!task || !sa)
    {
        return OS_Result_InvalidParams;
    }

    struct OS_DRIVER_StorageData *data = getStorageData (task);

    if (!data->queue.head)
    {
        DEBUG_Assert (!data->queue.elements);
        return OS_Result_Empty;
    }

    struct OS_DRIVER_StorageJob *job = (struct OS_DRIVER_StorageJob *)
                                                        data->queue.head;

    if (!QUEUE_DetachNode (&data->queue, data->queue.head))
    {
        return OS_Result_Error;
    }

    data->pending = job->access;
    *sa             = job->access;

    // Job slot is marked as free.
    job->access = NULL;

    return OS_Result_OK;
}


enum OS_Result OS_DRIVER_StorageJobDone (struct OS_TaskControl *task,
                                         struct OS_TaskDriverStorageAccess *sa,
                                         enum OS_Result result)
{
    if (!task || !sa)
    {
        return OS_Result_InvalidParams;
    }

    sa->result = result;

    struct OS_DRIVER_StorageData *data = getStorageData (task);

    if (result == OS_Result_OK)
    {
        ++ data->jobsSucceeded;
    }
    else
    {
        ++ data->jobsFailed;
    }

    if (sa->op == OS_TaskDriverOp_Read)
    {
        data->sectorsRead += sa->count;
    }
    else
    {
        data->sectorsWritten += sa->count;
    }

    // Release associate caller semaphore and set scheduler interrupt to pending
    // to resume caller execution.
    if (!DEBUG_Assert (SEMAPHORE_Release (sa->sem)))
    {
        return OS_Result_Error;
    }

    OS_SchedulerCallPending ();

    return OS_Result_OK;
}
