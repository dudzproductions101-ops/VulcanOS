; idt_flush.asm - Load the IDT register
;
; Trivial, but LIDT cannot be expressed from C, so it lives here
; alongside gdt_flush for the same reason: called from C, needs to
; be real assembly.

bits 64
section .text

global idt_flush
idt_flush:
    ; rdi = pointer to idt_ptr struct
    lidt [rdi]
    ret

; Marks this object as not requiring an executable stack. Without
; this, the linker assumes the opposite (a real security concern
; it's right to flag) and every VulcanOS binary would end up with
; W^X-violating stack permissions by default.
section .note.GNU-stack noalloc noexec nowrite progbits
