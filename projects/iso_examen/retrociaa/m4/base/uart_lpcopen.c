/*
    Copyright 2018 Santiago Germino (royconejo@gmail.com)

    Contibutors:
        {name/email}, {feature/bugfix}.

    RETRO-CIAA™ Library - UART portable basic functions for LPCOpen.

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
#include "uart_io.h"
#include "chip.h"


bool UART_Config (struct UART *u, uint32_t baudRate)
{
    if (!u || !u->handler)
    {
        return false;
    }

    LPC_USART_T *usart = (LPC_USART_T *)u->handler;

    // Periferico (MUX) ya configuado en board de LCPOpen
    // Aca solo se setean parametros
    Chip_UART_Init          (usart);
    Chip_UART_SetBaudFDR    (usart, baudRate);
    Chip_UART_ConfigData    (usart, UART_LCR_WLEN8 | UART_LCR_PARITY_DIS |
                             UART_LCR_SBS_1BIT);
    Chip_UART_TXEnable      (usart);
    return true;
}


uint32_t UART_GetByte (void *handler)
{
    if (!handler)
    {
        return UART_EOF;
    }

    LPC_USART_T *usart = (LPC_USART_T *)handler;
    if (Chip_UART_ReadLineStatus(usart) & UART_LSR_RDR)
    {
        return Chip_UART_ReadByte (usart);
    }
    return UART_EOF;
}


bool UART_PutByte (void *handler, uint8_t byte)
{
    if (!handler)
    {
        return false;
    }

    LPC_USART_T *usart = (LPC_USART_T *)handler;
    while ((Chip_UART_ReadLineStatus(usart) & UART_LSR_THRE) == 0)
    {
    }
    Chip_UART_SendByte (usart, byte);
    return true;
}
