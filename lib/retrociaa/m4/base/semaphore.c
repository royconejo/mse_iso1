/*
    Copyright 2019 Santiago Germino (royconejo@gmail.com)

    Contibutors:
        {name/email}, {feature/bugfix}.

    RETRO-CIAA™ Library - Semaphore object and functions.

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
#include "semaphore.h"
#include "debug.h"
#include "chip.h"       // CMSIS
#include <string.h>


bool SEMAPHORE_Init (struct SEMAPHORE *s, uint32_t resources,
                     uint32_t available)
{
    if (!s || !resources || available > resources)
    {
        return false;
    }

    memset (s, 0, sizeof(struct SEMAPHORE));

    s->resources    = resources;
    s->available    = available;

    return true;
}


bool SEMAPHORE_Acquire (struct SEMAPHORE *s)
{
    if (!s)
    {
        return false;
    }

    uint32_t value = __LDREXW (&s->available);

    if (value == 0)
    {
        return false;
    }

    -- value;

    if (__STREXW (value, &s->available))
    {
        return false;
    }

    return true;
}


bool SEMAPHORE_Release (struct SEMAPHORE *s)
{
    if (!s)
    {
        return false;
    }

    uint32_t value = __LDREXW (&s->available);

    if (value + 1 > s->resources)
    {
        return false;
    }

    ++ value;

    if (__STREXW (value, &s->available))
    {
        return false;
    }

    return true;
}


uint32_t SEMAPHORE_Available (struct SEMAPHORE *s)
{
    if (!s)
    {
        return 0;
    }

    return __LDREXW (&s->available);
}
