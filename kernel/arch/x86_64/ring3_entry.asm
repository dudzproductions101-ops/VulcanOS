; ring3_entry.asm - Ring-3 user mode entry point
; 
; ring3_enter_user_mode(rdi = entry_point, rsi = user_rsp)
; 
; Transition from ring 0 (kernel) to ring 3 (user mode).
; x86-64 privilege transition via iretq.

bits 64
section .text
global ring3_enter_user_mode

ring3_enter_user_mode:
    ; Disable interrupts during setup
    cli

    ; Prepare stack frame for iretq
    ; Layout (top to bottom):
    ; [SS (ring 3 data selector) + RPL 3]
    ; [RSP (user stack)]
    ; [RFLAGS (enable interrupts)]
    ; [CS (ring 3 code selector) + RPL 3]
    ; [RIP (entry point)]

    ; Push SS (ring 3 data segment + RPL 3)
    mov rax, 0x20 | 3   ; User data selector (0x20) with RPL 3
    push rax

    ; Push RSP (user stack pointer)
    push rsi

    ; Push RFLAGS (with IF enabled)
    pushfq
    pop rax
    or rax, 0x200       ; Set IF flag (enable interrupts)
    push rax

    ; Push CS (ring 3 code segment + RPL 3)
    mov rax, 0x18 | 3   ; User code selector (0x18) with RPL 3
    push rax

    ; Push RIP (entry point)
    push rdi

    ; Stack now ready for iretq
    ; iretq pops in reverse order and transitions to ring 3
    iretq

