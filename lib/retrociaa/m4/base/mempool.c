/*
    Copyright 2019 Santiago Germino (royconejo@gmail.com)

    Contibutors:
        {name/email}, {feature/bugfix}.

    RETRO-CIAA™ Library - 8 byte aligned static memory pool.

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
#include "mempool.h"
#include "debug.h"
#include <string.h>


#define MEMPOOL_BLOCK_SIGNATURE     0xACA1B10C


struct MEMPOOL_Block
{
    struct QUEUE_Node   node;
    uint32_t            size;
    const char          *description;
    uint32_t            signature;
}
ATTR_DataAlign8;


bool MEMPOOL_Init (struct MEMPOOL *m, uint32_t baseAddr, uint32_t size)
{
    if (!m)
    {
        return false;
    }

    // baseAddr must be aligned on a 8 byte boundary
    if (baseAddr & 0b111)
    {
        return false;
    }

    memset (m, 0, sizeof(struct MEMPOOL));

    m->baseAddr     = baseAddr;
    m->size         = size;
    m->used         = 0;

    return true;
}


void * MEMPOOL_Block (struct MEMPOOL *m, uint32_t blockSize,
                      const char *description)
{
    if (!m || !blockSize)
    {
        return NULL;
    }

    // blockSize must be expanded to contain a hidden MEMPOOL_Block and total
    // size must be multiple of 8.
    blockSize += sizeof (struct MEMPOOL_Block);
    blockSize += ATTR_RoundTo8 (blockSize);

    if (blockSize > (m->size - m->used))
    {
        return NULL;
    }

    const uint32_t NextBlockAddr    = m->baseAddr + m->used;
    struct MEMPOOL_Block *block     = (struct MEMPOOL_Block *) NextBlockAddr;

    memset (block, 0, blockSize);

    block->size         = blockSize;
    block->description  = description;
    block->signature    = MEMPOOL_BLOCK_SIGNATURE;

    m->used += blockSize;

    QUEUE_PushNode (&m->blocks, (struct QUEUE_Node *) block);

    // Return address will point at the end of the hidden block structure
    ++ block;

    // Make sure resulting block address is aligned on a 8 byte boundary
    DEBUG_Assert (!((uint32_t)block & 0b111));

    return (void *) block;
}


uint32_t MEMPOOL_BlockSize (void *b)
{
    if (!b)
    {
        return 0;
    }

    struct MEMPOOL_Block *block = (struct MEMPOOL_Block *) b;

    // Access the hidden block structure after the returned block pointer
    -- block;

    if (block->signature != MEMPOOL_BLOCK_SIGNATURE)
    {
        return 0;
    }

    // Returns usable size
    return (block->size - sizeof(struct MEMPOOL_Block));
}


uint32_t MEMPOOL_Available (struct MEMPOOL *m)
{
    return (m->size - m->used);
}
