/*
    Copyright 2018 Santiago Germino (royconejo@gmail.com)

    Contibutors:
        {name/email}, {feature/bugfix}.

    RETRO-CIAA™ Library - VARIANT type object and functions.

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
#include <stdint.h>
#include <stdbool.h>


enum VARIANT_Type
{
    VARIANT_TypeUint32 = 0,
    VARIANT_TypeInt32,
    VARIANT_TypeFloat,
    VARIANT_TypePointer,
    VARIANT_TypeString
};


struct VARIANT
{
    enum VARIANT_Type   type;
    char                conv[16];
    union
    {
        uint32_t    u;
        int32_t     i;
        float       f;
        void        *p;
        const char  *s;
    };
};


void        VARIANT_SetUint32       (struct VARIANT *v, const uint32_t u);
void        VARIANT_SetInt32        (struct VARIANT *v, const int32_t i);
void        VARIANT_SetFloat        (struct VARIANT *v, const float f);
void        VARIANT_SetPointer      (struct VARIANT *v, const void *p);
void        VARIANT_SetString       (struct VARIANT *v, const char *s);
uint32_t    VARIANT_ToUint32        (struct VARIANT *v);
int32_t     VARIANT_ToInt32         (struct VARIANT *v);
float       VARIANT_ToFloat         (struct VARIANT *v);
const char* VARIANT_ToString        (struct VARIANT *v);
bool        VARIANT_CmpStrings      (struct VARIANT *v, struct VARIANT *s);
bool        VARIANT_CmpUint32s      (struct VARIANT *v, struct VARIANT *s);
