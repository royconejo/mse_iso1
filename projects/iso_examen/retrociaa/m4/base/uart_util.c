/*
    Copyright 2018 Santiago Germino (royconejo@gmail.com)

    Contibutors:
        {name/email}, {feature/bugfix}.

    RETRO-CIAA™ Library - UART utility functions.

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
#include "uart_util.h"
#include "text.h"


/*
    Ventajas (para la documentacion):

    1)  Sustitucion dependiente del comodin, no de la posicion (permite
        i18n de strings).
    2)  Datos pasados como variants, no posiciones de memoria a interpretar.
    3)  Puede sustituir el mismo dato dos o mas veces.
*/

bool UART_PutMessageArgs (struct UART *u, const char *msg,
                          struct VARIANT argValues[], uint32_t argCount)
{
    if (!u || !msg || !argValues || !argCount)
    {
        return false;
    }

    uint32_t i = 0;

    while (msg[i])
    {
        const char Next = msg[i + 1];
        if (!Next)
        {
            UART_PutBinary (u, (uint8_t *)msg, i + 1);
            break;
        }

        if (msg[i] != '%')
        {
            ++ i;
            continue;
        }

        // msg[i] == '%' && Next
        UART_PutBinary (u, (uint8_t *)msg, i);  // {.. ..}%[Next]
        if (Next >= '1' && Next <= '9')         // %([1-9]{1})
        {
            const uint32_t arg = Next - 49;
            if (arg < argCount)
            {
                UART_PutMessage (u, VARIANT_ToString(&argValues[arg]));
            }
        }
        else if (Next == '%')
        {
            // "%%" = escape sequence for a single '%'
            UART_PutBinary (u, (uint8_t *)&Next, 1);
        }
        else {
            // Invalid char following '%'
            UART_PutMessage (u, TEXT_REPLACEMENTCHAR);
        }
        msg = &msg[i + 2];                      // %.[0]
        i = 0;
    }

    return true;
}

