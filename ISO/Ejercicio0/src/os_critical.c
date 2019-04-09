/*
    Copyright 2019 Santiago Germino (royconejo@gmail.com)

    RETRO-CIAA™ Library - Preemtive multitasking Operating System (RETRO-OS).
                          Non-user (internal) critical section functions.

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
#include "os_critical.h"
#include "debug.h"
#include "chip.h"       // CMSIS
#include <stdint.h>


static volatile uint32_t g_OS_CriticalSection   = 0;
static volatile uint32_t g_OS_SchedulingMisses  = 0;


void OS_SchedulerWakeup ()
{
    if (!g_OS_CriticalSection)
    {
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
        __DSB ();
        __ISB ();

        // TODO: chequera que aca efectivamente ya se haya llamado a interrupcion!
    }
    else
    {
        ++ g_OS_SchedulingMisses;
    }
}


void OS_ResetSchedulingMisses ()
{
    g_OS_SchedulingMisses = 0;
}


void OS_CriticalSection__ENTER ()
{
    DEBUG_Assert (!g_OS_CriticalSection);

    g_OS_CriticalSection = 1;
}


void OS_CriticalSection__LEAVE ()
{
    DEBUG_Assert (g_OS_CriticalSection);

    g_OS_CriticalSection = 0;

    if (g_OS_SchedulingMisses)
    {
        OS_SchedulerWakeup ();
    }
}


void OS_CriticalSection__CLEAR ()
{
    DEBUG_Assert (g_OS_CriticalSection);

    g_OS_CriticalSection = 0;
}


void OS_CriticalSection__RESET ()
{
    g_OS_CriticalSection = 0;
}
