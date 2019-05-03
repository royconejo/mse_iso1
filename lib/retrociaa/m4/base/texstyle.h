/*
    RETRO-CIAA™ Library
    Copyright 2018 Santiago Germino (royconejo@gmail.com)

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
#include "term.h"


#define TEXSTYLE_NL             "\r\n"
#define TEXSTYLE_PREFIX_INFO    "░░ "
#define TEXSTYLE_PREFIX_WARNING "▓▓ "
#define TEXSTYLE_PREFIX_ERROR   "▓▓ "
#define TEXSTYLE_PREFIX_GROUP   " ░ "

#define TEXSTYLE_INFO_BEGIN     TEXSTYLE_PREFIX_INFO
#define TEXSTYLE_INFO_END       TEXSTYLE_NL
#define TEXSTYLE_INFO(s)        TEXSTYLE_INFO_BEGIN s TEXSTYLE_INFO_END

#define TEXSTYLE_COLOR_BEGIN(c) TERM_FG_COLOR_##c \
                                TEXSTYLE_PREFIX_ERROR
#define TEXSTYLE_COLOR_END      TERM_NO_COLOR \
                                TEXSTYLE_NL
#define TEXSTYLE_COLOR(c,s)     TEXSTYLE_COLOR_BEGIN(c) s TEXSTYLE_COLOR_END

#define TEXSTYLE_WARNING(s)     TEXSTYLE_COLOR(BROWN,s)
#define TEXSTYLE_ERROR(s)       TEXSTYLE_COLOR(RED,s)
#define TEXSTYLE_OK(s)          TEXSTYLE_COLOR(BOLD_GREEN,s)
#define TEXSTYLE_CRITICAL(s)    TEXSTYLE_COLOR(BOLD_RED,s)
