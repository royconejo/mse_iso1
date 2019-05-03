/*
    Copyright 2018 Santiago Germino (royconejo@gmail.com)

    Contibutors:
        {name/email}, {feature/bugfix}.

    RETRO-CIAA™ Library - UART with CYCLIC buffer, object and functions.

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
#include "uart.h"
#include "uart_io.h"
#include <string.h>


bool UART_Init (struct UART *u, void *handler, uint32_t baudRate,
                uint8_t *recvBuffer, uint32_t recvBufferSize,
                uint8_t *sendBuffer, uint32_t sendBufferSize)
{
    if (!u || !recvBuffer || !recvBufferSize || !sendBuffer || !sendBufferSize)
    {
        return false;
    }

    memset (u, 0, sizeof(struct UART));

    CYCLIC_Init (&u->recv, recvBuffer, recvBufferSize);
    CYCLIC_Init (&u->send, sendBuffer, sendBufferSize);

    u->handler = handler;

    return UART_Config (u, baudRate);
}


uint32_t UART_SendPendingCount (struct UART *u)
{
    if (!u)
    {
        return 0;
    }

    return CYCLIC_Pending (&u->send);
}


uint32_t UART_RecvPendingCount (struct UART *u)
{
    if (!u)
    {
        return 0;
    }

    return CYCLIC_Pending (&u->recv);
}


bool UART_PutBinary (struct UART *u, const uint8_t *data,
                     uint32_t size)
{
    if (!u)
    {
        return false;
    }

    return CYCLIC_InFromBuffer (&u->send, data, size);
}


bool UART_PutMessage (struct UART *u, const char *msg)
{
    return UART_PutBinary (u, (uint8_t *)msg, strlen(msg));
}


uint32_t UART_Send (struct UART *u)
{
    if (!u)
    {
        return 0;
    }

    return CYCLIC_OutToStream (&u->send, UART_PutByte, u->handler,
                               UART_HW_FIFO_SIZE);
}


uint32_t UART_Recv (struct UART *u)
{
    if (!u)
    {
        return 0;
    }

    return CYCLIC_InFromStream (&u->recv, UART_GetByte, u->handler,
                                UART_EOF);
}


bool UART_RecvInjectByte (struct UART *u, uint8_t byte)
{
    if (!u)
    {
        return false;
    }

    return CYCLIC_In (&u->recv, byte);
}


uint8_t UART_RecvPeek (struct UART *u, uint32_t offset)
{
    if (!u)
    {
        return 0;
    }

    return CYCLIC_Peek (&u->recv, offset);
}


bool UART_RecvDiscardPending (struct UART *u)
{
    if (!u)
    {
        return false;
    }

    return CYCLIC_DiscardPending (&u->recv);
}
