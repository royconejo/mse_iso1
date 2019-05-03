/*
    Copyright 2019 Santiago Germino (royconejo@gmail.com)

    Contibutors:
        {name/email}, {feature/bugfix}.

    RETRO-CIAA™ Library - Preemtive multitasking Operating System (ReTrOS™).
                          Resource usage measurements and statistics.

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
#include "usage.h"
#include "opaque.h"
#include "../../base/debug.h"
#include "chip.h"       // CMSIS / SystemCoreClock

#include <string.h>


inline static void updateCurrentCpu (struct OS_USAGE_Cpu *uc,
                                     const OS_Cycles Cycles)
{
    uc->curCycles += Cycles;
    ++ uc->curSwitches;
}


inline static void updateCurrentMemory (struct OS_USAGE_Memory *um,
                                        const int32_t CurMem)
{
    um->curMedian += CurMem;

    if (um->curMin > CurMem)
    {
        um->curMin = CurMem;
    }

    if (um->curMax < CurMem)
    {
        um->curMax = CurMem;
    }

    ++ um->curMeasures;
}


inline static void updateLastCpu (struct OS_USAGE *u, struct OS_USAGE_Cpu *cu)
{
    cu->lastUsage       = cu->curCycles * u->cyclesPerTargetTicks;
    cu->lastCycles      = cu->curCycles;
    cu->lastSwitches    = cu->curSwitches;
    cu->curCycles       = 0;
    cu->curSwitches     = 0;
}


inline static void updateLastMemory (struct OS_USAGE *u,
                                     struct OS_USAGE_Memory *um,
                                     const int32_t CurMem,
                                     const uint32_t TotalMem)
{
    if (um->curMeasures)
    {
        um->lastMedian  = um->curMedian / um->curMeasures;
        um->lastMin     = um->curMin;
        um->lastMax     = um->curMax;
    }
    else
    {
        um->lastMedian  = CurMem;
        um->lastMin     = CurMem;
        um->lastMax     = CurMem;
    }

    um->lastUsage   = (float)um->lastMedian / (float)TotalMem;

    um->curMedian   = 0;
    um->curMin      = INT32_MAX;
    um->curMax      = INT32_MIN;
    um->curMeasures = 0;
}


enum OS_Result OS_USAGE_CpuReset (struct OS_USAGE_Cpu *uc)
{
    if (!uc)
    {
        return OS_Result_InvalidParams;
    }

    memset (uc, 0, sizeof(struct OS_USAGE_Cpu));

    return OS_Result_OK;
}


enum OS_Result OS_USAGE_MemoryReset (struct OS_USAGE_Memory *um)
{
    if (!um)
    {
        return OS_Result_InvalidParams;
    }

    memset (um, 0, sizeof(struct OS_USAGE_Memory));

    um->curMin  = INT32_MAX;
    um->curMax  = INT32_MIN;

    return OS_Result_OK;
}


enum OS_Result OS_USAGE_Init (struct OS_USAGE *u)
{
    if (!u)
    {
        return OS_Result_InvalidParams;
    }

    memset (u, 0, sizeof(struct OS_USAGE));

    u->targetTicksCount = OS_UsageDefaultTargetTicks;

    return OS_Result_OK;
}


bool OS_USAGE_UpdatingLastMeasures (struct OS_USAGE *u)
{
    if (!u)
    {
        return false;
    }

    return u->updateLastMeasures;
}


// Returns memory used by a task, including static amount reserved for its
// control structure.
int32_t OS_USAGE_GetUsedTaskMemory (void *taskBuffer)
{
    if (!taskBuffer)
    {
        return 0;
    }

    struct OS_TaskControl *task = (struct OS_TaskControl *) taskBuffer;

    int32_t Used = task->size - (task->sp - (int32_t)taskBuffer)
                    + sizeof(struct OS_TaskControl);

    DEBUG_Assert (Used >= sizeof(struct OS_TaskControl));

    return Used;
}


enum OS_Result OS_USAGE_SetTargetTicks (struct OS_USAGE *u, OS_Ticks ticks)
{
    if (!u)
    {
        return OS_Result_InvalidParams;
    }

    u->targetTicksCount = ticks;

    return OS_Result_OK;
}


enum OS_Result OS_USAGE_UpdateTarget (struct OS_USAGE *u, const OS_Ticks Now)
{
    if (!u)
    {
        return OS_Result_InvalidParams;
    }

    u->updateLastMeasures = false;

    // TargetTicksNext still in the future
    if (u->targetTicksNext > Now)
    {
        return OS_Result_OK;
    }

    // Update last measure only if this is not the first call to
    // UpdateTarget (no last measure to update then)
    if (u->targetTicksNext)
    {
        u->updateLastMeasures = true;
    }

    const uint32_t CountDiff = Now - u->targetTicksNext;
    u->cyclesPerTargetTicks  = 1.0f
                        / ((float) SystemCoreClock
                        / (float) (u->targetTicksCount + CountDiff)
                        * 1000000.0f / OS_GetTickPeriod_us ());
    // Try to keep discrete-time measurements
    u->targetTicksNext = Now + u->targetTicksCount - CountDiff;

    // IMPORTANT: trace logs will need to know the exact timestamp of when
    // measurements were taken.
    u->lastMeasurementPeriod = Now;

    return OS_Result_OK;
}


// This should only be used with the running task (the one running when the
// scheduler took over)
enum OS_Result OS_USAGE_UpdateCurrentMeasures (struct OS_USAGE *u,
                                               struct OS_USAGE_Cpu *uc,
                                               struct OS_USAGE_Memory *um,
                                               OS_Cycles cycles, int32_t memory)
{
    if (!u)
    {
        return OS_Result_InvalidParams;
    }

    if (uc)
    {
        updateCurrentCpu (uc, cycles);
    }

    if (um)
    {
        updateCurrentMemory (um, memory);
    }

    return OS_Result_OK;
}


enum OS_Result OS_USAGE_UpdateLastMeasures (struct OS_USAGE *u,
                                            struct OS_USAGE_Cpu *uc,
                                            struct OS_USAGE_Memory *um,
                                            int32_t curMem, uint32_t totalMem)
{
    if (!u)
    {
        return OS_Result_InvalidParams;
    }

    if (!u->updateLastMeasures)
    {
        return OS_Result_InvalidOperation;
    }

    if (uc)
    {
        updateLastCpu (u, uc);
    }

    if (um)
    {
        updateLastMemory (u, um, curMem, totalMem);
    }

    return OS_Result_OK;
}
