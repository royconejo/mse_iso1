#
#   Copyright 2018 Santiago Germino (royconejo@gmail.com)
#
#   Contibutors:
#       {name/email}, {feature/bugfix}.
#
#   RETRO-CIAA™ Library - Makefiles: OpenOCD debug/firmware flash adaptor.
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
    $(error [openocd.mk] Undefined SDK_ROOT)
endif

ifndef BIN_IMAGE
    $(error [openocd.mk] Undefined BIN_IMAGE)
endif

ifndef FLASH_BASE
    $(error [openocd.mk] Undefined FLASH_BASE)
endif

OPENOCD ?= openocd
OPENOCD_CFG ?= $(SDK_ROOT)/makefiles/flash/openocd_lpc4337.cfg

flash: prog
    ifeq (, $(shell which $(OPENOCD)))
	$(error [openocd.mk] OpenOCD executable '$(OPENOCD)' not found)
    endif

	openocd -f $(OPENOCD_CFG) \
	    -c "init" \
	    -c "halt 0" \
	    -c "flash write_image erase unlock $(BIN_IMAGE) $(FLASH_BASE) bin" \
	    -c "reset run" \
	    -c "shutdown"
