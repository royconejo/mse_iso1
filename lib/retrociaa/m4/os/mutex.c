/*
    Copyright 2019 Santiago Germino (royconejo@gmail.com)

    Contibutors:
        {name/email}, {feature/bugfix}.

    RETRO-CIAA™ Library - Preemtive multitasking Operating System (ReTrOS™).
                          Mutex object.

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
#include "mutex.h"
#include "api.h"

#include <string.h>


enum OS_Result OS_MUTEX_Init (struct OS_MUTEX *m)
{
    if (!m)
    {
        return OS_Result_InvalidParams;
    }

    memset (m, 0, sizeof(struct OS_MUTEX));

    if (!SEMAPHORE_Init (&m->sem, 1, 1))
    {
        return OS_Result_Error;
    }

    m->owner = OS_TaskSelf ();

    if (!m->owner)
    {
        return OS_Result_InvalidState;
    }

    return OS_Result_OK;
}


enum OS_Result OS_MUTEX_Lock (struct OS_MUTEX *m)
{
    if (!m)
    {
        return OS_Result_InvalidParams;
    }

    // Mutex already locked when asking to lock from the same task
    if (!SEMAPHORE_Available (&m->sem) && m->owner == OS_TaskSelf ())
    {
        return OS_Result_OK;
    }

    if (SEMAPHORE_Acquire (&m->sem))
    {
        m->owner = OS_TaskSelf ();
        return OS_Result_OK;
    }

    return OS_Result_Retry;
}


enum OS_Result OS_MUTEX_Unlock (struct OS_MUTEX *m)
{
    if (!m || !m->owner)
    {
        return OS_Result_InvalidParams;
    }

    // Only the owner can unlock a locked mutex
    if (m->owner != OS_TaskSelf ())
    {
        return OS_Result_InvalidCaller;
    }

    // Already unlocked
    if (SEMAPHORE_Available (&m->sem))
    {
        return OS_Result_OK;
    }

    // Unlock
    if (SEMAPHORE_Release (&m->sem))
    {
        m->owner = NULL;
        return OS_Result_OK;
    }

    return OS_Result_Retry;
}
