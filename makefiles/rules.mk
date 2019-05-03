#
#   Copyright 2018 Santiago Germino (royconejo@gmail.com)
#
#   Contibutors:
#       {name/email}, {feature/bugfix}.
#
#   RETRO-CIAA™ Library - Makefiles: Compiler rules.
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
ifndef PROJECT_NAME
    $(error [rules.mk] Undefined PROJECT_NAME)
endif

ifndef PROJECT_ROOT
    $(error [rules.mk] Undefined PROJECT_ROOT)
endif

ifndef SDK_ROOT
    $(error [rules.mk] Undefined SDK_ROOT)
endif

ifndef CPU_MODEL
    $(error [rules.mk] Undefined CPU_MODEL)
endif

ifndef FLASH_TOOL
    $(error [rules.mk] Undefined FLASH_TOOL)
endif

ifeq ($(DEBUG),yes)
    BUILD_ROOT ?= $(PROJECT_ROOT)/build/$(CC)/debug_$(CPU_MODEL)_O$(OLEVEL)
else
    BUILD_ROOT ?= $(PROJECT_ROOT)/build/$(CC)/$(CPU_MODEL)_O$(OLEVEL)
endif

EXECUTABLE := $(BUILD_ROOT)/$(PROJECT_NAME).elf
BIN_IMAGE := $(BUILD_ROOT)/$(PROJECT_NAME).bin
HEX_IMAGE := $(BUILD_ROOT)/$(PROJECT_NAME).hex
EXE_LISTING := $(BUILD_ROOT)/$(PROJECT_NAME).lst
EXE_MAP := $(BUILD_ROOT)/$(PROJECT_NAME).map

LDFLAGS += -Map=$(EXE_MAP)

ifneq (yes,$(VERBOSE))
    VN=@
endif

# It seems there are better options than calling mkdir
# http://stackoverflow.com/questions/1950926/create-directories-using-make-file
define compile =
    $(VN)mkdir -p $(3)
    $(if $(VN), @echo '[CC]' $(1))
    $(VN)$(CC) $(CFLAGS) -c $(1) -o $(2)
endef

define link =
    $(if $(VN), @echo '[LD]' $(1))
    $(VN)$(LD) -o $(1) $(OBJS) --start-group $(LIBS) --end-group $(LDFLAGS)
endef

define clean =
    $(VN)rm -f $(OBJS)
    $(VN)rm -f $(EXECUTABLE) $(BIN_IMAGE) $(HEX_IMAGE)
    $(VN)rm -f $(PROJECT_NAME).lst
    $(if $(VN), @echo '[Objects deleted]')
endef

# Relative sdk paths to relative current build
OBJS := $(foreach fp,$(OBJS),$(abspath $(fp)))
OBJS := $(patsubst $(abspath $(SDK_ROOT))/%,$(BUILD_ROOT)/%,$(OBJS))

prog: $(OBJS)
prog: $(BIN_IMAGE)

all: prog

$(BIN_IMAGE): $(EXECUTABLE)
	$(OBJCOPY) -v -O binary $^ $@
	$(OBJCOPY) -O ihex $^ $(HEX_IMAGE)
	$(OBJDUMP) -h -S -D $(EXECUTABLE) > $(EXE_LISTING)
	$(SIZE) $(EXECUTABLE)

$(EXECUTABLE): $(OBJS)
	$(call link,$@)

$(BUILD_ROOT)/%.o: $(SDK_ROOT)/%.c
	$(call compile,$<,$@,$(@D))

$(BUILD_ROOT)/%.o: $(SDK_ROOT)/%.S
	$(call compile,$<,$@,$(@D))

clean:
	$(call clean)

include $(SDK_ROOT)/makefiles/flash/$(FLASH_TOOL).mk

.PHONY: clean flash
