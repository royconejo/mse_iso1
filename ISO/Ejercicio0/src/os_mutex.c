/*
    Copyright 2019 Santiago Germino (royconejo@gmail.com)

    RETRO-CIAA™ Library - Preemtive multitasking Operating System (RETRO-OS).
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
#include "os_mutex.h"
#include "os.h"
#include <string.h>


bool OS_MUTEX_Init (struct OS_MUTEX *m)
{
    if (!m)
    {
        return false;
    }

    memset (m, 0, sizeof(struct OS_MUTEX));

    if (!SEMAPHORE_Init (&m->sem, 1, 0))
    {
        return false;
    }

    m->owner = OS_TaskSelf ();

    if (!m->owner)
    {
        return false;
    }

    return true;
}


bool OS_MUTEX_Lock (struct OS_MUTEX *m)
{
    if (!m)
    {
        return false;
    }

    // Mutex already locked when asking to lock from the same task
    if (!SEMAPHORE_Available (&m->sem) && m->owner == OS_TaskSelf ())
    {
        return true;
    }

    if (SEMAPHORE_Acquire (&m->sem))
    {
        m->owner = OS_TaskSelf ();
        return true;
    }

    return false;
}


bool OS_MUTEX_Unlock (struct OS_MUTEX *m)
{
    if (!m || !m->owner)
    {
        return false;
    }

    // Only the owner can unlock a locked mutex
    if (m->owner != OS_TaskSelf ())
    {
        return false;
    }

    // Already unlocked
    if (SEMAPHORE_Available (&m->sem))
    {
        return true;
    }

    if (SEMAPHORE_Release (&m->sem))
    {
        m->owner = NULL;
        return true;
    }

    return false;
}
