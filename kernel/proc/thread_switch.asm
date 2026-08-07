; thread_switch.asm - Low-level context switch
;
; Implements context_switch(struct thread_context *from, struct
; thread_context *to) exactly as declared in proc/thread.h. Must be
; assembly: C has no way to say "save my return address and stack
; pointer into this struct, then jump into a completely different
; call stack as if THAT stack's own call had just returned."
;
; Register layout matches struct thread_context field order exactly
; (rbx, rbp, r12, r13, r14, r15, rsp) -- if that struct is ever
; reordered, this file's offsets must be updated to match, and
; vice versa. There is no compiler-enforced link between the two;
; keeping them in sync is a manual invariant of this codebase.

bits 64
section .text

global context_switch
context_switch:
    ; System V AMD64: rdi = from, rsi = to

    ; Save the CURRENT thread's callee-saved registers into *from.
    ; rax/rcx/rdx/rsi/rdi/r8-r11 are caller-saved by ABI convention,
    ; so whatever C code called context_switch() is already
    ; responsible for their values surviving this call -- this
    ; function only needs to preserve what a normal `call` wouldn't
    ; already guarantee.
    mov [rdi + 0],  rbx
    mov [rdi + 8],  rbp
    mov [rdi + 16], r12
    mov [rdi + 24], r13
    mov [rdi + 32], r14
    mov [rdi + 40], r15
    mov [rdi + 48], rsp

    ; Restore the TARGET thread's registers from *to. After this
    ; point we are conceptually "inside" that thread's own call
    ; stack -- rsp now points into memory context_switch's own
    ; caller never touched.
    mov rbx, [rsi + 0]
    mov rbp, [rsi + 8]
    mov r12, [rsi + 16]
    mov r13, [rsi + 24]
    mov r14, [rsi + 32]
    mov r15, [rsi + 40]
    mov rsp, [rsi + 48]

    ; `ret` pops whatever is at the new rsp and jumps there. For a
    ; thread that has run before, that's the real return address
    ; from ITS prior call into context_switch (the mirror image of
    ; what we just did). For a thread running for the very first
    ; time, thread_create planted a synthetic return address here
    ; instead, pointing at thread_trampoline -- so this single `ret`
    ; correctly handles both "resume a previously-switched-out
    ; thread" and "start a brand new thread" without needing an
    ; if/else anywhere in this function.
    ret

; Marks this object as not requiring an executable stack (see the
; identical note in the arch/x86_64 .asm files -- same reasoning
; applies to every VulcanOS object file).
section .note.GNU-stack noalloc noexec nowrite progbits
