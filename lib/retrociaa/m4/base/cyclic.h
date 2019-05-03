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
#pragma once

#include "stream.h"
#include <stdint.h>
#include <stdbool.h>


struct CYCLIC
{
    uint8_t     *data;
    uint32_t    capacity;
    // in: producer / data input
    uint32_t    inIndex;
    // out: consumer / data output
    uint32_t    outIndex;
    // FYI only, not part of the algorithm
    uint32_t    reads;
    uint32_t    writes;
    uint32_t    overflows;
    uint32_t    peeks;
    uint32_t    discards;
};


bool        CYCLIC_Init             (struct CYCLIC *c, uint8_t *data,
                                     uint32_t capacity);
bool        CYCLIC_Reset            (struct CYCLIC *c);
uint32_t    CYCLIC_Pending          (struct CYCLIC *c);
bool        CYCLIC_In               (struct CYCLIC *c, const uint8_t data);
bool        CYCLIC_InFromBuffer     (struct CYCLIC *c, const uint8_t *data,
                                     uint32_t size);
uint32_t    CYCLIC_InFromStream     (struct CYCLIC *c,
                                     STREAM_ByteInFunc streamFunc,
                                     void *streamFuncHandler,
                                     uint32_t EOFSignal);
bool        CYCLIC_Out              (struct CYCLIC *c, uint8_t *data);
bool        CYCLIC_OutToBuffer      (struct CYCLIC *c, uint8_t *data,
                                     uint32_t count);
uint32_t    CYCLIC_OutToStream      (struct CYCLIC *c,
                                     STREAM_ByteOutFunc streamFunc,
                                     void *streamFuncHandler,
                                     uint32_t maxBytes);
uint8_t     CYCLIC_Peek             (struct CYCLIC *c, uint32_t offset);
bool        CYCLIC_PeekToBuffer     (struct CYCLIC *c, uint32_t offset,
                                     uint8_t *data, uint32_t size);
bool        CYCLIC_DiscardPending   (struct CYCLIC *c);
