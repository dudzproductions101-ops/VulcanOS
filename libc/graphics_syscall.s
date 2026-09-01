.intel_syntax noprefix

.section .text
.global graphics_info
.global graphics_clear
.global graphics_draw_rect
.global syscall_exec

graphics_info:
    mov rax, 10
    syscall
    ret

graphics_clear:
    mov rax, 11
    syscall
    ret

graphics_draw_rect:
    mov r10, rcx
    mov rax, 12
    syscall
    ret

syscall_exec:
    /* SYS_EXEC (20): execute user process in ring 3
     * Arguments:
     *   RDI = entry point (function pointer)
     * Returns:
     *   RAX = result code
     * Note: This syscall never returns normally.
     *       It switches to ring 3 and the process runs until exit.
     */
    mov rax, 20
    syscall
    ret
