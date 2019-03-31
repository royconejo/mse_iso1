/*
    RETRO-CIAA™ Library
    Copyright 2018 Santiago Germino (royconejo@gmail.com)

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
static SYSTICK_Ticks                g_tickRateMicro = 0;


void SysTick_Handler ()
{
    ++ g_ticks;

    if (g_tickHook)
    {
        g_tickHook (g_ticks);
    }
}


void SYSTICK_SetMicrosecondPeriod (SYSTICK_Ticks micro)
{
    SysTick_Config ((SystemCoreClock * micro) / 1000000);
    g_tickRateMicro = micro;

}


void SYSTICK_SetMillisecondPeriod (SYSTICK_Ticks milli)
{
    SysTick_Config ((SystemCoreClock * milli) / 1000);
    g_tickRateMicro = milli * 1000;
}


SYSTICK_Ticks SYSTICK_GetTickRateMicroseconds ()
{
    return g_tickRateMicro;
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
