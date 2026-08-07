; boot.asm - VulcanOS kernel entry point
;
; GRUB (via the Multiboot2 protocol) loads this kernel and hands
; control over in 32-bit protected mode with paging disabled. This
; file's job is the well-known but fiddly transition sequence every
; x86_64 kernel needs: build minimal identity-mapped page tables,
; enable PAE and long mode, load a 64-bit GDT, and far-jump into
; 64-bit code -- at which point kernel.c's kmain() takes over.
;
; VulcanOS does not skip this by using a Multiboot2-aware bootloader
; that starts in long mode directly, because no such standard exists;
; every long-mode kernel performs this same transition somewhere.
; What VulcanOS keeps original is everything *after* this handoff.

bits 32

section .multiboot2
align 8
mb2_header_start:
    dd 0xE85250D6                ; Multiboot2 magic
    dd 0                         ; architecture: 0 = i386/x86 protected mode
    dd mb2_header_end - mb2_header_start
    dd -(0xE85250D6 + 0 + (mb2_header_end - mb2_header_start)) & 0xFFFFFFFF

    ; end tag (required terminator)
    align 8
    dw 0
    dw 0
    dd 8
mb2_header_end:

section .bss
align 16
stack_bottom:
    resb 131072                  ; 128 KiB early boot stack (see VULCAN_BOOT_STACK_SIZE;
                                  ; increased from 16 KiB after a measured stack overflow
                                  ; found during vpkg bring-up -- see config.h's full
                                  ; explanation. Still an exact multiple of 4096, so the
                                  ; align 4096 immediately below needs zero padding bytes,
                                  ; same as before -- preserving the fix from the earlier
                                  ; NASM per-byte-warning bug (see PROJECT_STATUS.md's
                                  ; scheduler bring-up section for that history).
stack_top:

; Page tables need 4096-byte alignment for the CPU's page-table-base
; requirements. Placed immediately after the stack (before the small
; saved-registers block below) so this alignment jump only has to
; cover a small, fixed gap -- not one that grows/shrinks based on
; unrelated declarations placed in between, which is what previously
; caused NASM to emit one warning per padding byte when a 3.7 KiB gap
; needed filling (see the -w+other spam this used to produce).
align 4096
p4_table: resb 4096              ; PML4
p3_table: resb 4096              ; PDPT
p2_table: resb 4096              ; PD (2 MiB pages, identity-maps first 1 GiB)

; Fixed storage for the values GRUB hands us in eax/ebx. NOT kept on
; the stack across the check_*/setup_page_tables/enable_paging call
; sequence below: each of those is a `call`, which pushes/pops its
; own return address, and stacking a manual save UNDER an unknown
; number of subsequent call/ret pairs is fragile by construction --
; exactly this fragility previously caused eax/ebx to be popped back
; at the wrong stack depth and width once long_mode_start needed
; them (32-bit pushes here, but the original code did 64-bit pops
; in 64-bit mode after several intervening calls, corrupting both
; values). Fixed memory locations have no such ambiguity: written
; once immediately on entry, read once right before the jump to
; long mode, with nothing stack-position-dependent in between.
; Placed after the (already 4096-aligned) page tables rather than
; before them, so no further alignment padding is needed here.
mb2_magic_saved:  resd 1
mb2_info_saved:   resd 1

section .text
global _start
extern kmain

_start:
    cli
    mov esp, stack_top

    ; Multiboot2 hands us: eax = magic (0x36D76289), ebx = pointer to
    ; the boot info structure. Saved to fixed storage rather than the
    ; stack -- see the comment at mb2_magic_saved above for why.
    mov [mb2_magic_saved], eax
    mov [mb2_info_saved], ebx

    call check_multiboot
    call check_cpuid
    call check_long_mode

    call setup_page_tables
    call enable_paging

    lgdt [gdt64.pointer]
    jmp gdt64.code_seg:long_mode_start

; --- Sanity checks -------------------------------------------------

check_multiboot:
    cmp eax, 0x36D76289
    jne .no_multiboot
    ret
.no_multiboot:
    mov al, "M"
    jmp error

check_cpuid:
    ; Flip bit 21 (ID) in EFLAGS; if it stays flipped after a
    ; round-trip through the stack, CPUID is supported.
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    cmp eax, ecx
    je .no_cpuid
    ret
.no_cpuid:
    mov al, "C"
    jmp error

check_long_mode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long_mode
    ret
.no_long_mode:
    mov al, "L"
    jmp error

; --- Paging setup ----------------------------------------------------
; Identity-maps the first 1 GiB using 2 MiB pages. This is deliberately
; temporary scaffolding: kmain transitions to VulcanOS's real virtual
; memory layout (higher-half kernel, see VULCAN_KERNEL_VBASE) once the
; mm subsystem initializes in Phase 2. Booting here first avoids
; needing a fully general page-table walker before the kernel can even
; print its first line.

setup_page_tables:
    mov eax, p3_table
    or eax, 0b11                 ; present, writable
    mov [p4_table], eax

    mov eax, p2_table
    or eax, 0b11
    mov [p3_table], eax

    xor ecx, ecx
.map_p2_table:
    mov eax, 0x200000            ; 2 MiB
    mul ecx
    or eax, 0b10000011           ; present, writable, huge page
    mov [p2_table + ecx * 8], eax

    inc ecx
    cmp ecx, 512
    jne .map_p2_table
    ret

enable_paging:
    mov eax, p4_table
    mov cr3, eax

    mov eax, cr4
    or eax, 1 << 5                ; PAE
    mov cr4, eax

    mov ecx, 0xC0000080           ; EFER MSR
    rdmsr
    or eax, 1 << 8                ; LME (long mode enable)
    wrmsr

    mov eax, cr0
    or eax, 1 << 31               ; PG (paging)
    mov cr0, eax
    ret

; Prints "ERR: X" where X is the code in al, then halts. Used only
; for the pre-flight checks above, which run before printk exists.
error:
    mov dword [0xB8000], 0x4F524F45
    mov dword [0xB8004], 0x4F3A4F52
    mov dword [0xB8008], 0x4F204F20
    mov byte  [0xB800A], al
    hlt

section .rodata
gdt64:
    dq 0                                              ; null descriptor
.code_seg: equ $ - gdt64
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53)          ; 64-bit code segment
.pointer:
    dw $ - gdt64 - 1
    dq gdt64

bits 64
section .text
long_mode_start:
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; System V AMD64 calling convention: first two integer args go
    ; in rdi, rsi. Loaded from the fixed storage _start wrote them
    ; to (see mb2_magic_saved above), not popped off the stack --
    ; the stack approach previously corrupted both values, since
    ; several `call`/`ret` pairs (check_multiboot, check_cpuid, ...)
    ; ran in between the original push and this pop, at 32-bit push
    ; width followed by a 64-bit pop.
    ;
    ; Writing to the 32-bit sub-register (edi/esi) rather than the
    ; full 64-bit rdi/rsi is deliberate, not a shortcut: x86_64
    ; guarantees any write to a 32-bit register zeroes the upper 32
    ; bits of its parent 64-bit register, so this is the standard,
    ; correct way to zero-extend a 32-bit value into a clean 64-bit
    ; one -- avoiding exactly the kind of upper-bits garbage that
    ; caused this bug in the first place.
    mov edi, [mb2_magic_saved]    ; arg0: multiboot magic
    mov esi, [mb2_info_saved]     ; arg1: multiboot info ptr

    call kmain
    ; kmain does not return under normal operation; if it somehow
    ; does, halt rather than execute whatever garbage follows.
    cli
.hang:
    hlt
    jmp .hang

; Marks this object as not requiring an executable stack. Without
; this, the linker assumes the opposite (a real security concern
; it's right to flag) and every VulcanOS binary would end up with
; W^X-violating stack permissions by default.
section .note.GNU-stack noalloc noexec nowrite progbits
