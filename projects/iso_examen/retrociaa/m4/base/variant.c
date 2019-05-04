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
#include "variant.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void VARIANT_SetUint32 (struct VARIANT *v, const uint32_t u)
{
    if (v)
    {
        v->u    = u;
        v->type = VARIANT_TypeUint32;
    }
}


void VARIANT_SetInt32 (struct VARIANT *v, const int32_t i)
{
    if (v)
    {
        v->i    = i;
        v->type = VARIANT_TypeInt32;
    }
}


void VARIANT_SetFloat (struct VARIANT *v, const float f)
{
    if (v)
    {
        v->f    = f;
        v->type = VARIANT_TypeFloat;
    }
}


void VARIANT_SetPointer (struct VARIANT *v, const void *p)
{
    if (v)
    {
        v->p    = (void *) p;
        v->type = VARIANT_TypePointer;
    }
}


void VARIANT_SetString (struct VARIANT *v, const char *s)
{
    if (v)
    {
        v->s    = s;
        v->type = VARIANT_TypeString;
    }
}


uint32_t VARIANT_ToUint32 (struct VARIANT *v)
{
    if (!v)
    {
        return 0;
    }

    switch (v->type)
    {
        case VARIANT_TypeUint32:
            return v->u;

        case VARIANT_TypeInt32:
            return (uint32_t) v->i;

        case VARIANT_TypeFloat:
            return (uint32_t) v->f;

        case VARIANT_TypePointer:
            return (uint32_t) v->p;

        case VARIANT_TypeString:
            return (uint32_t) strtoul (v->s, NULL, 0);
    }
    return 0;
}


int32_t VARIANT_ToInt32 (struct VARIANT *v)
{
    if (!v)
    {
        return 0;
    }

    switch (v->type)
    {
        case VARIANT_TypeUint32:
            return (int32_t) v->u;

        case VARIANT_TypeInt32:
            return v->i;

        case VARIANT_TypeFloat:
            return (int32_t) v->f;

        case VARIANT_TypePointer:
            break;

        case VARIANT_TypeString:
            return (int32_t) strtol (v->s, NULL, 0);
    }
    return 0;
}


float VARIANT_ToFloat (struct VARIANT *v)
{
    if (!v)
    {
        return 0;
    }

    switch (v->type)
    {
        case VARIANT_TypeUint32:
            return (float) v->u;

        case VARIANT_TypeInt32:
            return (float) v->i;

        case VARIANT_TypeFloat:
            return v->f;

        case VARIANT_TypePointer:
            break;

        case VARIANT_TypeString:
            return strtof (v->s, NULL);
    }
    return 0;
}


const char* VARIANT_ToString (struct VARIANT *v)
{
    if (!v)
    {
        return NULL;
    }

    v->conv[0] = 'N';
    v->conv[1] = '/';
    v->conv[2] = 'A';
    v->conv[3] = '\0';

    switch (v->type)
    {
        case VARIANT_TypeUint32:
            snprintf (v->conv, sizeof(v->conv), "%lu", v->u);
            break;

        case VARIANT_TypeInt32:
            snprintf (v->conv, sizeof(v->conv), "%li", v->i);
            break;

        case VARIANT_TypeFloat:
            snprintf (v->conv, sizeof(v->conv), "%f", (double)v->f);
            break;

        case VARIANT_TypePointer:
            snprintf (v->conv, sizeof(v->conv), "%p", v->p);
            break;

        case VARIANT_TypeString:
            return v->s;
    }

    return v->conv;
}


bool VARIANT_CmpStrings (struct VARIANT *v, struct VARIANT *s)
{
    if (!v || !s || !v->s || !s->s)
    {
        return false;
    }

    if (v->s == s->s)
    {
        return true;
    }

    uint32_t i = 0;
    while (v->s[i] && s->s[i])
    {
        if (v->s[i] != s->s[i])
        {
            return false;
        }
        ++ i;
    }

    if (!v->s[i] && !s->s[i])
    {
        return true;
    }

    return false;
}


bool VARIANT_CmpUint32s (struct VARIANT *v, struct VARIANT *s)
{
    if (!v || !s)
    {
        return false;
    }

    return (VARIANT_ToUint32(v) == VARIANT_ToUint32(s));
}
