bits 64
section .text

global gdt_flush
gdt_flush:
    lgdt [rdi]

    mov ax, 0x10          
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    push qword 0x08              
    lea rax, [rel .reload_cs]
    push rax
    retfq
.reload_cs:
    ret

global tss_flush
tss_flush:
    mov ax, 0x28         
    ltr ax
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
