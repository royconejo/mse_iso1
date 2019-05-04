/*
    Copyright 2018 Santiago Germino (royconejo@gmail.com)
    
    Contibutors:
        {name/email}, {feature/bugfix}.

    RETRO-CIAA™ Library - Cyclic/ring buffer optimized for 2^n lenghts.    

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
#include "cyclic.h"
#include <string.h>


bool CYCLIC_Init (struct CYCLIC *c, uint8_t *data, uint32_t capacity)
{
    if (!c || !data || !capacity)
    {
        return false;
    }

    // Capacity debe ser potencia de 2
    if (capacity & (capacity - 1))
    {
        return false;
    }

    memset (c, 0, sizeof(struct CYCLIC));

    c->data       = data;
    c->capacity   = capacity;
    return true;
}


bool CYCLIC_Reset (struct CYCLIC *c)
{
    if (!c || !c->data || !c->capacity)
    {
        return false;
    }
    
    memset (c->data, 0, c->capacity);
    
    c->inIndex  = 0;
    c->outIndex = 0;
    return true;
}


uint32_t CYCLIC_Pending (struct CYCLIC *c)
{
    if (!c)
    {
        return 0;
    }

    return (c->inIndex - c->outIndex) & (c->capacity - 1);
}


bool CYCLIC_In (struct CYCLIC *c, const uint8_t data)
{
    return CYCLIC_InFromBuffer (c, &data, 1);
}


bool CYCLIC_InFromBuffer (struct CYCLIC *c, const uint8_t *data, uint32_t size)
{
    if (!c || !data || !size)
    {
        return false;
    }

    const uint32_t Pending = CYCLIC_Pending (c);

    for (uint32_t i = size; i; --i)
    {
        c->data[c->inIndex] = *data ++;
        c->inIndex = (c->inIndex + 1) & (c->capacity - 1);
    }

    c->writes += size;
    if (size > c->capacity - Pending)
    {
        c->overflows += size - (c->capacity - Pending);
    }

    return true;
}


uint32_t CYCLIC_InFromStream (struct CYCLIC *c,
                              STREAM_ByteInFunc streamFunc,
                              void *streamFuncHandler, uint32_t EOFSignal)
{
    if (!c || !streamFunc)
    {
        return 0;
    }

    const uint32_t Pending = CYCLIC_Pending (c);

    uint32_t    count = 0;
    uint32_t    data;

    while ((data = streamFunc (streamFuncHandler)) != EOFSignal)
    {
        c->data[c->inIndex] = (uint8_t) data;
        c->inIndex = (c->inIndex + 1) & (c->capacity - 1);
        ++ count;
    }

    c->writes += count;
    if (count > c->capacity - Pending)
    {
        c->overflows += count - (c->capacity - Pending);
    }

    return count;
}


bool CYCLIC_Out (struct CYCLIC *c, uint8_t *data)
{
    return CYCLIC_OutToBuffer (c, data, 1);
}


// Call CYCLIC_Pending() before to know how much data is available
bool CYCLIC_OutToBuffer (struct CYCLIC *c, uint8_t *data, uint32_t count)
{
    if (!c || !data || !count)
    {
        return false;
    }

    const uint32_t Pending = CYCLIC_Pending (c);
    if (!Pending)
    {
        return true;
    }

    if (count > Pending)
    {
        count = Pending;
    }

    c->reads += count;

    while (count --)
    {
        *data ++ = c->data[c->outIndex];
        c->outIndex = (c->outIndex + 1) & (c->capacity - 1);
    }

    return true;
}


uint32_t CYCLIC_OutToStream (struct CYCLIC *c,
                             STREAM_ByteOutFunc streamFunc,
                             void *streamFuncHandler, uint32_t maxBytes)
{
    if (!c || !streamFunc)
    {
        return 0;
    }

    uint32_t pending = CYCLIC_Pending (c);
    if (!pending)
    {
        return 0;
    }

    if (maxBytes && pending > maxBytes)
    {
        pending = maxBytes;
    }

    c->reads += pending;

    const uint32_t BytesSent = pending;
    while (pending --)
    {
        streamFunc (streamFuncHandler, c->data[c->outIndex]);
        c->outIndex = (c->outIndex + 1) & (c->capacity - 1);
    }

    return BytesSent;
}


// Call CYCLIC_Pending() before to know how much data is available
uint8_t CYCLIC_Peek (struct CYCLIC *c, uint32_t offset)
{
    if (!c)
    {
        return 0;
    }

    ++ c->peeks;
    return c->data[(c->outIndex + offset) & (c->capacity - 1)];
}


// Call CYCLIC_Pending() before to know how much data is available
bool CYCLIC_PeekToBuffer (struct CYCLIC *c, uint32_t offset, uint8_t *data,
                          uint32_t size)
{
    if (!c || !data || !size)
    {
        return false;
    }
    
    for (uint32_t i = 0; i < size; ++i)
    {
        *data ++ = c->data[(c->outIndex + offset + i) & (c->capacity - 1)];
    }
    
    c->peeks += size;
    return true;
}


bool CYCLIC_DiscardPending (struct CYCLIC *c)
{
    if (!c)
    {
        return false;
    }

    const uint32_t Pending = CYCLIC_Pending (c);
    c->outIndex = (c->outIndex + Pending) & (c->capacity - 1);
    c->discards += Pending;
    return true;
}
