; isr_stubs.asm - ISR/IRQ stub table
;
; (Originally interrupts.asm; renamed to avoid a build-system object-
; path collision with interrupts.c, and because "isr_stubs" more
; precisely names what this file owns: the fixed set of numbered
; entry trampolines the CPU jumps to. interrupts.c owns *policy*
; (PIC remap, dispatch, IRQ handler registration) -- this file owns
; only "normalize whatever the CPU pushed into one common shape and
; call into C". Two different responsibilities, two files.
;
; The CPU only knows how to push a fixed hardware frame (and, for
; some exceptions, an error code) before jumping to a handler address
; taken from the IDT -- it does not know or care about our C calling
; convention. Every stub below normalizes that into the single shape
; isr_dispatch() (in interrupts.c) expects: push a fake error code of
; 0 for vectors that don't have one, push the vector number so the C
; dispatcher can tell handlers apart, save all general-purpose
; registers, then call in. This file must be kept in exact sync with
; struct interrupt_frame in interrupts.h.

bits 64
section .text
extern isr_dispatch

; NOTE ON PUSH ORDER: struct interrupt_frame (interrupts.h) declares
; `vector` before `error_code`, meaning vector must end up at the
; LOWER stack address once both are pushed (stack grows down, and a
; C struct's first-declared field sits at its base/lowest address).
; Since the *last* thing pushed ends up at the lowest address, vector
; must be pushed AFTER error_code. Get this backwards and every
; handler reads the vector number where it expects the error code
; and vice versa -- verified against the struct layout, not assumed.
%macro ISR_NOERR 1
global isr%1
isr%1:
    push qword 0        ; fake error code, keeps frame layout uniform
    push qword %1        ; vector number (pushed last -> lowest address)
    jmp isr_common_stub
%endmacro

%macro ISR_ERR 1
global isr%1
isr%1:
    ; CPU already pushed error_code before jumping here.
    push qword %1        ; vector number (pushed last -> lowest address)
    jmp isr_common_stub
%endmacro

%macro IRQ_STUB 2
global irq%1
irq%1:
    push qword 0
    push qword %2
    jmp isr_common_stub
%endmacro

; CPU exceptions 0-31. Vectors 8, 10-14, 17 push a hardware error
; code automatically (Intel SDM Vol. 3, Table 6-1); the rest do not,
; so ISR_NOERR synthesizes a zero to keep the frame shape identical
; either way.
; Hardware IRQs, remapped to vectors 32-47 by pic_remap() in
; interrupts.c. Only timer (32) and keyboard (33) are wired to a
; named stub for this bring-up milestone; the remaining legacy IRQ
; lines (cascade, RTC, etc.) will get stubs as their drivers land.
; CPU exception stubs 0-31.
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR 8
ISR_NOERR 9
ISR_ERR 10
ISR_ERR 11
ISR_ERR 12
ISR_ERR 13
ISR_ERR 14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

IRQ_STUB 0, 32   ; timer
IRQ_STUB 1, 33   ; keyboard
ISR_NOERR 128

isr_common_stub:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

        mov rdi, rsp          ; arg0 = pointer to interrupt_frame
    call isr_dispatch

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 16            ; discard vector + error_code
    iretq

; Marks this object as not requiring an executable stack. Without
; this, the linker assumes the opposite (a real security concern
; it's right to flag) and every VulcanOS binary would end up with
; W^X-violating stack permissions by default.
section .note.GNU-stack noalloc noexec nowrite progbits
