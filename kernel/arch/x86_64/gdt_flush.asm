; gdt_flush.asm - Reload segment registers after installing a new GDT
;
; Split into its own tiny file rather than folded into boot.asm:
; boot.asm runs exactly once during early bring-up, while gdt_flush
; is called from gdt_install() in C and may be re-invoked later (for
; example if VulcanOS ever needs to rebuild the GDT per-CPU during
; SMP bring-up). Keeping it separate means it's callable from C
; without dragging in the one-shot boot sequence.
;
; The CS-reload sequence below (push selector, push target address,
; RETFQ) is the standard long-mode idiom: a plain far JMP to an
; immediate 64-bit address isn't encodable, and IRETQ would require
; also pushing RFLAGS/SS/RSP for no benefit here since we're not
; changing privilege level. RETFQ only needs CS:RIP.

bits 64
section .text

global gdt_flush
gdt_flush:
    ; rdi = pointer to gdt_ptr struct (System V AMD64 calling convention)
    lgdt [rdi]

    mov ax, 0x10          ; GDT_SEL_KERNEL_DATA
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    push qword 0x08              ; GDT_SEL_KERNEL_CODE
    lea rax, [rel .reload_cs]
    push rax
    retfq
.reload_cs:
    ret

global tss_flush
tss_flush:
    mov ax, 0x28           ; GDT_SEL_TSS
    ltr ax
    ret

; Marks this object as not requiring an executable stack. Without
; this, the linker assumes the opposite (a real security concern
; it's right to flag) and every VulcanOS binary would end up with
; W^X-violating stack permissions by default.
section .note.GNU-stack noalloc noexec nowrite progbits
