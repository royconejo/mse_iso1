#
#   Copyright 2018 Santiago Germino (royconejo@gmail.com)
#
#   Contibutors:
#       {name/email}, {feature/bugfix}.
#
#   RETRO-CIAA™ Library - Makefiles: LPCOpen library.
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
    $(error [lpcopen.mk] Undefined SDK_ROOT)
endif

ifndef CPU_MODEL
    $(error [lpcopen.mk] Undefined CPU_MODEL)
endif

LPCOPEN ?= $(SDK_ROOT)/lib/lpcopen

ifdef LPCOPEN_VER
    LPCOPEN := $(LPCOPEN)_$(LPCOPEN_VER)
endif

ifeq ($(wildcard $(LPCOPEN)/.),)
    $(error [lpcopen.mk] LPCOpen library not found)
endif

ifeq ($(VERBOSE),yes)
    $(info [lpcopen.mk] LPCOpen library path = '$(LPCOPEN)')
endif

CFLAGS += -D__CODE_RED -D__USE_LPCOPEN -D__LPC43XX__

# Standad sysinit (with or without board)
OBJS += $(SDK_ROOT)/lib/lpcopen_sysinit/sysinit.o

# Path to CMSIS (required)
CFLAGS += -I$(LPCOPEN)/CMSIS/Include

# Chip support library (M0 & M4)
LPCOPEN_CHIP := $(LPCOPEN)/lpc_core/lpc_chip

# Path to LPCOpen chip files, common code and usbd_rom
CFLAGS += -I$(LPCOPEN_CHIP)/chip_18xx_43xx
CFLAGS += -I$(LPCOPEN_CHIP)/chip_common
CFLAGS += -I$(LPCOPEN_CHIP)/usbd_rom

# Core dependant paths
ifeq ($(CPU_MODEL), cortex-m4)
#   CFLAGS += -DLPC43_MULTICORE_M0APP -D__MULTICORE_MASTER \
#             -D__MULTICORE_MASTER_SLAVE_M0APP
    CFLAGS += -I$(LPCOPEN_CHIP)/chip_18xx_43xx/config_43xx
    # fpuInit()
    OBJS += $(LPCOPEN_CHIP)/chip_common/fpu_init.o
else
    ifeq ($(CPU_MODEL), cortex-m0)
#       CFLAGS += -D__MULTICORE_M0APP -DCORE_M0APP
        CFLAGS += -I$(LPCOPEN_CHIP)/chip_18xx_43xx/config_43xx_m0app
    else
        $(error [lpcopen.mk] Unexpected CPU_MODEL = '$(CPU_MODEL)')
    endif
endif

# Selectable chip drivers
define include_driver
    OBJS += $(LPCOPEN_CHIP)/chip_18xx_43xx/$(1)_18xx_43xx.o
endef

# Board required chip drivers
ifneq ($(LPCOPEN_BOARD_CHIP_DRIVERS),)
    LPCOPEN_CHIP_DRIVERS := $(filter-out $(LPCOPEN_BOARD_CHIP_DRIVERS), $(LPCOPEN_CHIP_DRIVERS))
    LPCOPEN_CHIP_DRIVERS += $(LPCOPEN_BOARD_CHIP_DRIVERS)
endif

ifeq ($(LPCOPEN_CHIP_DRIVERS),)
    ifeq ($(VERBOSE),yes)
        $(info [lpcopen.mk] Using CMSIS only)
    endif
else
    ifeq ($(VERBOSE),yes)
        $(info [lpcopen.mk] Using chip drivers = '$(LPCOPEN_CHIP_DRIVERS)')
    endif

    # uart chip code requires ring_buffer functions
    ifneq ($(filter uart,$(LPCOPEN_CHIP_DRIVERS)),)
        OBJS += $(LPCOPEN_CHIP)/chip_common/ring_buffer.o
    endif

    $(foreach name,$(LPCOPEN_CHIP_DRIVERS),$(eval $(call include_driver,$(name))))
endif

define include_hostusbclass
    OBJS += $(LPCOPEN_USBLIB)/Drivers/USB/Class/Host/$(1)ClassHost.o
endef

ifneq ($(LPCOPEN_USB_HOST_CLASSES),)
    $(info [lpcopen.mk] Using USB host library)
    # LPCUSBLib
    LPCOPEN_USBLIB ?= $(LPCOPEN)/LPCUSBLib
    # Host only
    CFLAGS += -DUSB_HOST_ONLY
    
    CFLAGS += -I$(LPCOPEN_USBLIB)
    CFLAGS += -I$(LPCOPEN_USBLIB)/Common
    CFLAGS += -I$(LPCOPEN_USBLIB)/Drivers/USB
    CFLAGS += -I$(LPCOPEN_USBLIB)/Drivers/USB/Class
    CFLAGS += -I$(LPCOPEN_USBLIB)/Drivers/USB/Class/Common
    CFLAGS += -I$(LPCOPEN_USBLIB)/Drivers/USB/Class/Host
    CFLAGS += -I$(LPCOPEN_USBLIB)/Drivers/USB/Core
    CFLAGS += -I$(LPCOPEN_USBLIB)/Drivers/USB/Core/HAL
    CFLAGS += -I$(LPCOPEN_USBLIB)/Drivers/USB/Core/HAL/LPC18XX
    CFLAGS += -I$(LPCOPEN_USBLIB)/Drivers/USB/Core/HCD
    CFLAGS += -I$(LPCOPEN_USBLIB)/Drivers/USB/Core/HCD/EHCI

    # HAL: 18xx == 43xx
    OBJS += $(LPCOPEN_USBLIB)/Drivers/USB/Class/Common/HIDParser.o \
            $(LPCOPEN_USBLIB)/Drivers/USB/Core/ConfigDescriptor.o \
            $(LPCOPEN_USBLIB)/Drivers/USB/Core/Device.o \
            $(LPCOPEN_USBLIB)/Drivers/USB/Core/DeviceStandardReq.o \
            $(LPCOPEN_USBLIB)/Drivers/USB/Core/Endpoint.o \
            $(LPCOPEN_USBLIB)/Drivers/USB/Core/EndpointStream.o \
            $(LPCOPEN_USBLIB)/Drivers/USB/Core/Events.o \
            $(LPCOPEN_USBLIB)/Drivers/USB/Core/Host.o \
            $(LPCOPEN_USBLIB)/Drivers/USB/Core/HostStandardReq.o \
            $(LPCOPEN_USBLIB)/Drivers/USB/Core/Pipe.o \
            $(LPCOPEN_USBLIB)/Drivers/USB/Core/PipeStream.o \
            $(LPCOPEN_USBLIB)/Drivers/USB/Core/USBController.o \
            $(LPCOPEN_USBLIB)/Drivers/USB/Core/USBMemory.o \
            $(LPCOPEN_USBLIB)/Drivers/USB/Core/USBTask.o \
            $(LPCOPEN_USBLIB)/Drivers/USB/Core/HAL/LPC18XX/HAL_LPC18xx.o \
            $(LPCOPEN_USBLIB)/Drivers/USB/Core/HCD/HCD.o \
            $(LPCOPEN_USBLIB)/Drivers/USB/Core/HCD/EHCI/EHCI.o

    LPCOPEN_USB_HOST := $(SDK_ROOT)/lib/lpcopen_lpcusblib_host
    CFLAGS += -I$(LPCOPEN_USB_HOST)
#   ahora usa -lnosys
#   OBJS += $(LPCOPEN_USB_HOST)/syscalls.o

    # Audio, CDC, HID, MassStorage, MIDI, Printer, RNDIS, StillImage (see path contents)
    $(foreach name,$(LPCOPEN_USB_HOST_CLASSES),$(eval $(call include_hostusbclass,$(name))))

    # LPCUSBLib host with mass storage class is available
    ifneq ($(filter MassStorage,$(LPCOPEN_USB_HOST_CLASSES)),)
        CFLAGS += -DLPCUSBLIB_HOST_MASS_STORAGE
    endif
endif
