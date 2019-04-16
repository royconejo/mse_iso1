/*
    Copyright 2019 Santiago Germino (royconejo@gmail.com)

    RETRO-CIAA™ Library - Preemtive multitasking Operating System (ReTrOS™).
                          SVC, PendSV handlers for syscalls and context switch.

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
#define CONTROL_PSP     0x00000002

.syntax unified
.thumb
.text

.extern OS_Scheduler
.extern OS_SyscallHandler

.global OS_Syscall
.global SVC_Handler
.global PendSV_Handler


.thumb_func
    OS_Syscall:
            mrs      r3, control
            tst      r3, CONTROL_PSP
            beq      .syscallError
            svc      0
            bx       lr
        .syscallError:
            mov      r0, #1             @ return OS_Result_InvalidCall
            bx       lr


.thumb_func
    SVC_Handler:
            tst      lr, EXC_RETURN_PSP
            bne      .fromPSP           @ Must have been called from a PSP task
            bkpt     #0
            pop      {r0}               @ Pop original R0 value on MSP
            mov      r0, #1             @ OS_Result_InvalidCall
            push     {r0}               @ Push return value to R0 on MSP
            bx       lr
        .fromPSP:
            push     {lr}
            bl       OS_SyscallHandler
            pop      {lr}
            mrs      r12, psp
            ldmfd    r12!, {r1}         @ Pop original R0 value on PSP
            stmfd    r12!, {r0}         @ Push return value to R0 on PSP
            msr      psp, r12           @ Update PSP (not really needed)
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
            clrex
            mov      r2, #0
            ldr      r3, =DWT_CYCCNT
            ldr      r1, [r3]           @ Store cycle count in R1
            str      r2, [r3]           @ Zero cycle counter
            bl       OS_Scheduler       @ R0: currentSp, R1: taskCycles
            mov      r2, #0
            ldr      r3, =DWT_CYCCNT
            str      r2, [r3]           @ Zero cycle counter
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
