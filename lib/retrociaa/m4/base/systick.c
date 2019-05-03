/*
    Copyright 2019 Santiago Germino (royconejo@gmail.com)

    Contibutors:
        {name/email}, {feature/bugfix}.

    RETRO-CIAA™ Library - SYSTICK functions and event handler.

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
#include "systick.h"
#include "chip.h"   // CMSIS


static volatile SYSTICK_Ticks       g_ticks         = 0;
static volatile SYSTICK_HookFunc    g_tickHook      = NULL;
static SYSTICK_Ticks                g_tickPeriod_us = 0;


void SysTick_Handler ()
{
    ++ g_ticks;

    if (g_tickHook)
    {
        g_tickHook (g_ticks);
    }
}


void SYSTICK_SetPeriod_us (SYSTICK_Ticks us)
{
    SysTick_Config ((SystemCoreClock * us) / 1000000);
    g_tickPeriod_us = us;
}


void SYSTICK_SetPeriod_ms (SYSTICK_Ticks ms)
{
    SysTick_Config ((SystemCoreClock * ms) / 1000);
    g_tickPeriod_us = ms * 1000;
}


inline SYSTICK_Ticks SYSTICK_GetPeriod_us ()
{
    return g_tickPeriod_us;
}


inline SYSTICK_Ticks SYSTICK_Now ()
{
    return g_ticks;
}


SYSTICK_HookFunc SYSTICK_SetHook (SYSTICK_HookFunc func)
{
   SYSTICK_HookFunc old = g_tickHook;
   g_tickHook = func;
   return old;
}
