#
#   Copyright 2019 Santiago Germino (royconejo@gmail.com)
#
#   Contibutors:
#       {name/email}, {feature/bugfix}.
#
#   RETRO-CIAA™ Library - Makefiles: Project.
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
PROJECT_NAME := $(notdir $(shell pwd))
PROJECT_ROOT ?= .
SDK_ROOT = $(PROJECT_ROOT)/../..
VERBOSE ?= no
OLEVEL ?= 0
DEBUG ?= yes
FLASH_TOOL ?= openocd
MAKEFILES := boards/lpcopen_edu_retrociaa_m4 lib/retrociaa toolchain rules
FATFS_VER := 013c
LPCOPEN_VER := 302
LPCOPEN_CHIP_DRIVERS := gpio clock rtc i2s i2cm gpdma
# LPCOPEN_USB_HOST_CLASSES := MassStorage
RETRO_CIAA_BASE_APP := mempool uart cyclic text uart_lpcopen uart_util variant

# Project include paths
CFLAGS += -I$(PROJECT_ROOT)

# Project files
OBJS += \
    $(PROJECT_ROOT)/main.o \
    $(PROJECT_ROOT)/appdata.o \
    $(PROJECT_ROOT)/fault.o

$(foreach name,$(MAKEFILES),$(eval include $(SDK_ROOT)/makefiles/$(name).mk))
