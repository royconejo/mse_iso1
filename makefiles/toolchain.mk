#
#   Copyright 2018 Santiago Germino (royconejo@gmail.com)
#
#   Contibutors:
#       {name/email}, {feature/bugfix}.
#
#   RETRO-CIAA™ Library - Makefiles: Toolchain selection and options.
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
ifndef CPU_MODEL
    $(error [toolchain.mk] Undefined CPU_MODEL)
endif

# Default values
VERBOSE ?= no
DEBUG ?= yes
OLEVEL ?= 0
CLIBS += c gcc nosys

# Toolchain configuration (GNU Tools for ARM Embedded Processors)
CROSS_COMPILE ?= arm-none-eabi-
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
GDB := $(CROSS_COMPILE)gdb
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump
SIZE := $(CROSS_COMPILE)size

# Basic configurations
CFLAGS += --specs=nosys.specs
CFLAGS += -fno-hosted -nostartfiles -nodefaultlibs -nostdlib
CFLAGS += -mthumb -mcpu=$(CPU_MODEL)
CFLAGS += -std=c99 -Wall -Winline -ggdb3
CFLAGS += -O$(OLEVEL)
CFLAGS += -fdata-sections -ffunction-sections

# Debug information
ifeq ($(DEBUG),yes)
    CFLAGS += -g -DDEBUG
else
    # -flto
   # CFLAGS += -ffast-math
   # CFLAGS += -fno-common
endif

# Libraries
define get_library_path
    $(shell dirname $(shell $(CC) $(CFLAGS) -print-file-name=$(1)))
endef

$(foreach name,$(CLIBS), $(eval LDFLAGS += -L $(call get_library_path,lib$(name).a)))
$(foreach name,$(CLIBS), $(eval LIBS += -l$(name)))

LDFLAGS += --gc-sections --print-memory-usage
