/*
    Copyright 2018 Santiago Germino (royconejo@gmail.com)
    Based on code by Elm-ChaN, FatFs library.
    
    RETRO-CIAA™ Library storage low level driver definitions.

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

// STORAGE_Status, STORAGE_Result and IOCTRL values are equivalents to the FatFs
// library own values for disk interface. This is intended to ensure direct 
// interoperability between RETRO-CIAA low level storage funcions and FatFs.

#define STORAGE_Status_Not_Initialized      0x00000001u
#define STORAGE_Status_No_Disk              0x00000002u
#define STORAGE_Status_Write_Protected      0x00000004u

typedef uint32_t STORAGE_Status;


enum STORAGE_Result
{
    STORAGE_Result_Ok                   = 0,
    STORAGE_Result_ReadWriteError       = 1,
    STORAGE_Result_WriteProtected       = 2,
    STORAGE_Result_NotReady             = 3,
    STORAGE_Result_InvalidParameter     = 4,
    STORAGE_Result_Timeout              = 5
};


// Command codes for {DEVICE}_StorageIoCtl()

// Generic command (Used by FatFs)
#define CTRL_SYNC			0   // Complete pending write (FF_FS_READONLY == 0)
#define GET_SECTOR_COUNT	1	// Get media size (FF_USE_MKFS == 1)
#define GET_SECTOR_SIZE		2	// Get sector size (FF_MAX_SS != FF_MIN_SS)
#define GET_BLOCK_SIZE		3	// Get erase block size (FF_USE_MKFS == 1)
#define CTRL_TRIM			4	// Inform device that the data on the block of 
                                // sectors is no longer used (FF_USE_TRIM == 1)

// Generic command (Not used by FatFs)
#define CTRL_POWER			5	// Get/Set power status
#define CTRL_LOCK			6	// Lock/Unlock media removal
#define CTRL_EJECT			7	// Eject media
#define CTRL_FORMAT			8	// Create physical format on the media

// CTRL_POWER sub commands
#define CTRL_POWER_OFF      0
#define CTRL_POWER_ON       1
#define CTRL_POWER_STATUS   2
