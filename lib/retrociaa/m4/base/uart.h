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
#pragma once
#include "cyclic.h"
#include <stdint.h>
#include <stdbool.h>


#ifndef UART_HW_FIFO_SIZE
    #define UART_HW_FIFO_SIZE       16
#endif

// IMPORTANTE: Los valores deben ser 2^X!
#ifndef UART_RECV_BUFFER_SIZE
    #define UART_RECV_BUFFER_SIZE   256
#endif

#ifndef UART_SEND_BUFFER_SIZE
    #define UART_SEND_BUFFER_SIZE   2048
#endif


struct UART
{
    struct CYCLIC   recv;
    struct CYCLIC   send;
    // Platform dependant UART handler
    void            *handler;
};


bool        UART_Init               (struct UART *u, void *handler,
                                     uint32_t baudRate,
                                     uint8_t *recvBuffer,
                                     uint32_t recvBufferSize,
                                     uint8_t *sendBuffer,
                                     uint32_t sendBufferSize);
uint32_t    UART_SendPendingCount   (struct UART *u);
uint32_t    UART_RecvPendingCount   (struct UART *u);
bool        UART_PutBinary          (struct UART *u,
                                     const uint8_t *data, uint32_t size);
bool        UART_PutMessage         (struct UART *u, const char *msg);
uint32_t    UART_Send               (struct UART *u);
uint32_t    UART_Recv               (struct UART *u);
bool        UART_RecvInjectByte     (struct UART *u, uint8_t byte);
uint8_t     UART_RecvPeek           (struct UART *u, uint32_t offset);
bool        UART_RecvDiscardPending (struct UART *u);
