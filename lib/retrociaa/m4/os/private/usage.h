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
#pragma once

#include "../api.h"

#include <stdint.h>
#include <stdbool.h>


typedef uint64_t    OS_Cycles;

#define OS_UsageDefaultTargetTicks      1000


struct OS_USAGE
{
    uint32_t        targetTicksCount;
    uint32_t        targetTicksNext;
    float           cyclesPerTargetTicks;
    bool            updateLastMeasures;
    OS_Ticks        lastMeasurementPeriod;
};


struct OS_USAGE_Cpu
{
    OS_Cycles       curCycles;
    uint32_t        curSwitches;
    OS_Cycles       lastCycles;
    uint32_t        lastSwitches;
    float           lastUsage;
};


struct OS_USAGE_Memory
{
    int32_t         curMedian;
    int32_t         curMin;
    int32_t         curMax;
    int32_t         curMeasures;
    int32_t         lastMedian;
    int32_t         lastMin;
    int32_t         lastMax;
    float           lastUsage;
};


enum OS_Result  OS_USAGE_CpuReset               (struct OS_USAGE_Cpu *uc);
enum OS_Result  OS_USAGE_MemoryReset            (struct OS_USAGE_Memory *um);

enum OS_Result  OS_USAGE_Init                   (struct OS_USAGE *u);
bool            OS_USAGE_UpdatingLastMeasures   (struct OS_USAGE *u);
int32_t         OS_USAGE_GetUsedTaskMemory   (void *taskBuffer);
enum OS_Result  OS_USAGE_SetTargetTicks         (struct OS_USAGE *u,
                                                 OS_Ticks ticks);
enum OS_Result  OS_USAGE_UpdateTarget           (struct OS_USAGE *u,
                                                 const OS_Ticks Now);
enum OS_Result  OS_USAGE_UpdateCurrentMeasures  (struct OS_USAGE *u,
                                                 struct OS_USAGE_Cpu *uc,
                                                 struct OS_USAGE_Memory *um,
                                                 OS_Cycles cycles,
                                                 int32_t memory);
enum OS_Result  OS_USAGE_UpdateLastMeasures     (struct OS_USAGE *u,
                                                 struct OS_USAGE_Cpu *uc,
                                                 struct OS_USAGE_Memory *um,
                                                 int32_t curMem,
                                                 uint32_t totalMem);
