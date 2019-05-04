/*
    Copyright 2019 Santiago Germino (royconejo@gmail.com)

    Contibutors:
        {name/email}, {feature/bugfix}.

    RETRO-CIAA™ Library - Queue object and functions.

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
#include "queue.h"
#include "debug.h"
#include <string.h>


static void nodePush (struct QUEUE_Node *lastNode,
                      struct QUEUE_Node *newNode)
{
    if (!lastNode)
    {
        return;
    }

    lastNode->next = newNode;
    newNode->prev  = lastNode;
}


static void nodeDetach (struct QUEUE_Node *node)
{
    // Detaching node from the chain
    if (node->prev)
    {
        node->prev->next = node->next;
    }

    if (node->next)
    {
        node->next->prev = node->prev;
    }

    node->prev = NULL;
    node->next = NULL;
}


bool QUEUE_Init (struct QUEUE *queue)
{
    if (!queue)
    {
        return false;
    }

    memset (queue, 0, sizeof(struct QUEUE));

    return true;
}


bool QUEUE_PushNode (struct QUEUE *queue, struct QUEUE_Node *node)
{
    if (!queue || !node)
    {
        return false;
    }

    if (!queue->head)
    {
        DEBUG_Assert (!queue->tail);
        queue->head = node;
    }
    else
    {
        DEBUG_Assert (queue->tail);
        nodePush (queue->tail, node);
    }

    queue->tail = node;
    ++ queue->elements;

    return true;
}


bool QUEUE_DetachNode (struct QUEUE *queue, struct QUEUE_Node *node)
{
    if (!queue || !node)
    {
        return false;
    }

    DEBUG_Assert ((queue->head && queue->tail)
                    || (!queue->head && !queue->tail));

    DEBUG_Assert (queue->elements);

    const bool NodeIsHead = (queue->head == node);
    const bool NodeIsTail = (queue->tail == node);

    // Removing the only node in the queue.
    if (NodeIsHead && NodeIsTail)
    {
        DEBUG_Assert (!node->prev && !node->next);
        queue->head = NULL;
        queue->tail = NULL;
    }
    // Removing the first node.
    else if (NodeIsHead)
    {
        DEBUG_Assert (node->next);
        queue->head = node->next;
    }
    // Removing the last node.
    else if (NodeIsTail)
    {
        DEBUG_Assert (node->prev);
        queue->tail = node->prev;
    }

    nodeDetach (node);
    -- queue->elements;

    return true;
}
