#
#   Copyright 2018 Santiago Germino (royconejo@gmail.com)
#
#   Contibutors:
#       {name/email}, {feature/bugfix}.
#
#   RETRO-CIAA™ Library - Makefiles: RETRO-CIAA library (stripped down version).
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
    $(error [retrociaa.mk] Undefined SDK_ROOT)
endif

ifndef CPU_MODEL
    $(error [retrociaa.mk] Undefined CPU_MODEL)
endif

ifndef LPCOPEN
    $(error [retrociaa.mk] This library requires LPCOpen)
endif

RETRO_CIAA ?= $(SDK_ROOT)/lib/retrociaa

ifdef RETRO_CIAA_VER
    RETRO_CIAA := $(RETRO_CIAA)_$(RETRO_CIAA_VER)
endif

ifeq ($(wildcard $(RETRO_CIAA)/.),)
    $(error [retrociaa.mk] RETRO-CIAA library not found)
endif


CFLAGS += -I$(SDK_ROOT)


# Path to common library
RETRO_CIAA_COMMON := $(RETRO_CIAA)/common

define include_module
    OBJS += $(RETRO_CIAA)/$(1)/$(2).o
endef

# Core dependant paths & files
ifeq ($(CPU_MODEL), cortex-m4)
    RETRO_CIAA := $(RETRO_CIAA)/m4

    # Required base modules for ReTrOS
    RETRO_CIAA_BASE := systick attr debug queue semaphore

    ifneq ($(RETRO_CIAA_BASE_APP),)
        RETRO_CIAA_BASE := $(filter-out $(RETRO_CIAA_BASE_APP), $(RETRO_CIAA_BASE))
        RETRO_CIAA_BASE += $(RETRO_CIAA_BASE_APP)
    endif

    ifeq ($(VERBOSE),yes)
        $(info [retrociaa.mk] Using M4/BASE modules = '$(RETRO_CIAA_BASE)')
    endif

    # Base modules
    $(foreach name,$(RETRO_CIAA_BASE),$(eval $(call include_module,base,$(name))))

    # ReTrOS (RETRO-CIAA Real Time Operating System)
    OBJS += $(RETRO_CIAA)/os/api.o \
            $(RETRO_CIAA)/os/mutex.o \
            $(RETRO_CIAA)/os/scheduler.o \
            $(RETRO_CIAA)/os/port/ticks.o \
            $(RETRO_CIAA)/os/private/handlers.o \
            $(RETRO_CIAA)/os/private/opaque.o \
            $(RETRO_CIAA)/os/private/runtime.o \
            $(RETRO_CIAA)/os/private/syscall.o \
            $(RETRO_CIAA)/os/private/usage.o \
            $(RETRO_CIAA)/os/private/driver/storage.o

    # No Video, Audio, Storage or Input modules in this stripped down version
    
else
    # No support for Cortex-M0 Core (No video)
    $(error [retrociaa.mk] Unexpected CPU_MODEL = '$(CPU_MODEL)')
endif


ifeq ($(VERBOSE),yes)
    $(info [retrociaa.mk] RETRO-CIAA common library path = '$(RETRO_CIAA_COMMON)')
    $(info [retrociaa.mk] RETRO-CIAA core library path = '$(RETRO_CIAA)')
endif
