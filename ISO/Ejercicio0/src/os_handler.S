/*
    Copyright 2019 Santiago Germino (royconejo@gmail.com)

    RETRO-CIAA™ Library - Preemtive multitasking Operating System (ReTrOS™).
                          SVC, PendSV handlers for syscall and context switch.

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
#define DWT_CYCCNT	0xE0001004
#define EXC_RETURN_FPU  0x00000010
#define EXC_RETURN_PSP  0x00000004

.syntax unified
.thumb
.text

.extern OS_GetNextStackPointer
.extern OS_SystemCallHandler

.global OS_SystemCall
.global SVC_Handler
.global PendSV_Handler


.thumb_func
    OS_SystemCall:
            svc   0
            bx    lr


.thumb_func
    SVC_Handler:
            tst      lr, EXC_RETURN_PSP
            bne      .fromPspChecked    @ Must have been called from a PSP task
            bkpt     #0
            pop      {r0}               @ Pop original R0 value on MSP
            mov      r0, #1             @ OS_Result_InvalidCall
            push     {r0}               @ Push return value to R0 on MSP
            bx       lr
        .fromPspChecked:
            mrs      r12, psp           @ Store high registers from caller task
            tst      lr, EXC_RETURN_FPU
            it       eq
            vstmdbeq r12!, {s16-s31}
            stmfd    r12!, {r4-r11}
            msr      psp, r12
            isb
            push     {lr}
            bl       OS_SystemCallHandler
            pop      {lr}
            push     {r0}               @ Push return value to MSP
            mrs      r12, psp
            ldmfd    r12!, {r4-r11}
            tst      lr, EXC_RETURN_FPU
            it       eq
            vldmiaeq r12!, {s16-s31}
            ldmfd    r12!, {r0}         @ Pop original R0 value on PSP
            pop      {r0}               @ restore return value to R0
            stmfd    r12!, {r0}         @ Push return value to R0 on PSP
            msr      psp, r12
            isb
            bx       lr


.thumb_func
    PendSV_Handler:
            mrs      r0, psp
            cbnz     r0, .storePSP
            @ psp == 0, first scheduler run, store original msp.
        .pushMSP:
            tst      lr, EXC_RETURN_FPU
            it       eq
            vpusheq  {s16-s31}
            push     {r4-r11, lr}
            b        .nextPSP
        .storePSP:
            tst      lr, EXC_RETURN_FPU
            it       eq
            vstmdbeq r0!, {s16-s31}
            stmfd    r0!, {r4-r11, lr}
        .nextPSP:
            ldr      r1, =DWT_CYCCNT
            ldr      r1, [r1]
            bl       OS_GetNextStackPointer
            ldr      r2, =0
            ldr      r1, =DWT_CYCCNT
            str      r2, [r1]
            @ psp == 0, scheduler shut down, back to msp
            cbz      r0, .recoverMSP
        .updatePSP:
            ldmfd    r0!, {r4-r11, lr}
            tst      lr, EXC_RETURN_FPU
            it       eq
            vldmiaeq r0!, {s16-s31}
            msr      psp, r0
            isb
            b        .back
        .recoverMSP:
            pop      {r4-r11, lr}
            tst      lr, EXC_RETURN_FPU
            it       eq
            vpopeq   {s16-s31}
        .back:
            bx       lr
