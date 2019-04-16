#pragma once

#include "os.h"
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
