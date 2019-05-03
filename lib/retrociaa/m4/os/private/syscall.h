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
#pragma once

#include "../api.h"
#include "../../base/semaphore.h"
#include "../../storage/misc.h"


enum OS_Syscall
{
    ATTR_EnumForceUint32 (OS_Syscall),
    OS_Syscall_TaskBootEnded = 0,
    OS_Syscall_TaskStart,
    OS_Syscall_TaskYield,
    OS_Syscall_TaskWaitForSignal,
    OS_Syscall_TaskDelayFrom,
    OS_Syscall_TaskPeriodicDelay,
    OS_Syscall_TaskDriverStorageAccess,
    OS_Syscall_TaskTerminate,
    OS_Syscall_Terminate
};


// SysCall params
struct OS_TaskStart
{
    void                    *buffer;
    uint32_t                bufferSize;
    OS_Task                 func;
    OS_TaskParam            param;
    enum OS_TaskPriority    priority;
    const char              *description;
    enum OS_TaskType        type;
    void                    *initParams;
};


struct OS_TaskWaitForSignal
{
    struct OS_TaskControl   *task;
    enum OS_TaskSignalType  sigType;
    void                    *sigObject;
    OS_Ticks                start;
    OS_Ticks                timeout;
};


struct OS_TaskDelayFrom
{
    OS_Ticks                ticks;
    OS_Ticks                from;
};


enum OS_TaskDriverOp
{
    OS_TaskDriverOp_Read,
    OS_TaskDriverOp_Write,
    OS_TaskDriverOP__COUNT,
    OS_TaskDriverOp_Recv    = OS_TaskDriverOp_Read,
    OS_TaskDriverOp_Send    = OS_TaskDriverOp_Write
};


struct OS_TaskDriverStorageAccess
{
    const char              *description;
    enum OS_TaskDriverOp    op;
    uint8_t                 *buf;
    uint32_t                sector;
    uint32_t                count;
    uint32_t                processed;
    OS_Ticks                timeout;
    struct SEMAPHORE        *sem;
    enum STORAGE_Result     result;
};


struct OS_TaskDriverComAccess
{
    const char              *description;
    enum OS_TaskDriverOp    op;
    uint8_t                 *buf;
    uint32_t                count;
    uint32_t                processed;
    OS_Ticks                timeout;
    struct SEMAPHORE        *sem;
    enum OS_Result          result;
};


struct OS_TaskTerminate
{
    struct OS_TaskControl   *task;
    OS_TaskRetVal           retVal;
};


extern enum OS_Result   OS_Syscall          (enum OS_Syscall call,
                                             void *params);
enum OS_Result          OS_SyscallBoot      (enum OS_RunMode runMode,
                                             OS_Task bootTask,
                                             OS_TaskParam bootParam);
enum OS_Result          OS_SyscallShutdown  ();


void    OS_SyscallTaskStartInit (struct OS_TaskStart *ts, void *buffer,
                                 uint32_t bufferSize, OS_Task func,
                                 void *funcParam, enum OS_TaskPriority priority,
                                 const char *description, enum OS_TaskType type,
                                 void *initParams);
