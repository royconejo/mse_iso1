#
#   Copyright 2018 Santiago Germino (royconejo@gmail.com)
#
#   Contibutors:
#       {name/email}, {feature/bugfix}.
#
#   RETRO-CIAA™ Library - Makefiles: Support for EDU-CIAA-NXP Poncho board on
#                                    Cortex-M4F Core.
#
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions are met:
#
#   1.  Redistributions of source code must retain the above copyright notice,
#       this list of conditions and the following disclaimer.
#
#   2.  Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#
#   3.  Neither the name of the copyright holder nor the names of its
#       contributors may be used to endorse or promote products derived from
#       this software without specific prior written permission.
#
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
#   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#   POSSIBILITY OF SUCH DAMAGE.
#
ifndef SDK_ROOT
    $(error [edu-retrociaa_m4.mk] Undefined SDK_ROOT)
endif

# EDU-CIAA-NXP mcu
include $(SDK_ROOT)/makefiles/cores/lpc43xx/lpc4337jbd144_m4.mk

# drivers needed for m4 board code
LPCOPEN_BOARD_CHIP_DRIVERS := i2c ssp adc uart sysinit clock chip
 
include $(SDK_ROOT)/makefiles/lib/lpcopen.mk

LPCOPEN_BOARD := $(SDK_ROOT)/lib/lpcopen_edu_retrociaa_board

CFLAGS += -I$(LPCOPEN_BOARD)

OBJS += \
    $(LPCOPEN_BOARD)/board.o \
    $(LPCOPEN_BOARD)/board_sysinit.o
