# VulcanOS Status

A precise, implementation‑verified snapshot of what is **real code** versus **intentional gaps** in VulcanOS.

---

## 1. Implemented and Boot‑Verified

These components exist, compile, and have been **boot‑verified** in QEMU.

### 1.1 Architecture / Core

- **Multiboot2 header + long‑mode transition**  
  `kernel/arch/x86_64/boot.asm`
- **GDT + TSS**  
  `kernel/arch/x86_64/gdt.c`, `gdt_flush.asm`
- **IDT (256 entries)**  
  `kernel/arch/x86_64/idt.c`, `idt_flush.asm`
- **ISR/IRQ stubs (exceptions + timer + keyboard)**  
  `kernel/arch/x86_64/isr_stubs.asm`
- **Interrupt dispatch + PIC remap**  
  `kernel/arch/x86_64/interrupts.c`
- **CPU identification (CPUID, long‑mode check)**  
  `kernel/arch/x86_64/cpu.c`

### 1.2 Console & Devices

- **VGA text console**  
  `kernel/drivers/console/console.c`
- **PIT timer (100 Hz)**  
  `kernel/drivers/timer/timer.c`
- **PS/2 keyboard (scancode set 1)**  
  `kernel/drivers/input/keyboard.c`

### 1.3 Kernel Core

- **Kernel printf**  
  `kernel/core/printk.c`
- **Panic handler**  
  `kernel/core/panic.c`
- **kmain / bring‑up sequence**  
  `kernel/core/kernel.c`
- **Linker script (1 MiB load, page‑aligned)**  
  `kernel/linker.ld`
- **Kernel build system (freestanding GCC)**  
  `kernel/Makefile`, top‑level `Makefile`

### 1.4 Memory Management

- **Multiboot2 tag parser**  
  `kernel/core/multiboot2.c`, `kernel/include/multiboot2.h`
- **Physical memory manager (bitmap)**  
  `kernel/mm/pmm.c`
- **Paging / virtual memory (4‑level, NX, higher‑half)**  
  `kernel/mm/paging.c`
- **Kernel heap allocator (first‑fit, grows via paging)**  
  `kernel/mm/allocator.c`

### 1.5 Processes & Scheduling

- **Thread control blocks + context switch**  
  `kernel/proc/thread.c`, `kernel/proc/thread_switch.asm`
- **Process control blocks (shared kernel page tables)**  
  `kernel/proc/process.c`
- **Round‑robin scheduler (5‑tick slices, preemption)**  
  `kernel/proc/scheduler.c`

### 1.6 Filesystem

- **VFS layer**  
  `kernel/fs/vfs.c`
- **vulcanfs (RAM‑resident backend)**  
  `kernel/fs/vulcanfs.c`
- **Generic inode helpers**  
  `kernel/fs/inode.c`

### 1.7 libc

- **String functions**  
  `libc/string.c`
- **Memory + process primitives**  
  `libc/stdlib.c`
- **I/O + printf + `vulcan_readdir`**  
  `libc/stdio.c`

### 1.8 Userland

- **init (PID 2, adapter to `vulsh`)**  
  `user/init/init.c`
- **vulsh (shell)**  
  `user/shell/vulsh.c`
- **Core utilities: `ls`, `cat`, `echo`**  
  `user/bin/ls.c`, `user/bin/cat.c`, `user/bin/echo.c`

---

## 2. Explicitly Not Started (Empty by Design)

These paths are **real gaps**, intentionally left empty and tracked.

- **Bootloader**  
  `boot/*`  
  Native BIOS/UEFI bootloader to replace GRUB.
- **Package / ISO tooling**  
  `tools/image.py`, `tools/mkfs.c`  
  Custom ISO builder and host‑side vulcanfs image creator.
  
---

## 3. Interim Design Decisions

### 3.1 vulcanfs Is RAM‑Resident

- No storage drivers (ATA/AHCI/NVMe) yet.
- VFS + one complete backend (vulcanfs) implemented first.
- All file data lives in kernel heap; no persistence across reboot.
- Future disk‑backed FS will plug into `inode_ops` without changing VFS.

### 3.2 libc Calls Kernel Directly

- No ring‑3, no syscall boundary.
- libc functions (`malloc`, `open`, `read`, `write`, `exit`, …) call kernel functions directly.
- Signatures already match future userland API to avoid later churn.

### 3.3 Userland Processes Are Linked‑In Threads

- `init`, `vulsh`, `ls`, `cat`, `echo` are compiled into the kernel image.
- Started as scheduler threads, not loaded from disk.
- Command dispatch in `vulsh` is direct function calls, not `exec()`.

---

## 4. Bugs Found and Fixed

### 4.1 Boot / Register Handling

- **Register corruption across boot handoff**  
  Fixed by saving Multiboot2 magic/info to `.bss` and reloading in long mode.

- **NASM warning storm on `.bss` alignment**  
  Fixed by reordering `.bss` so page tables follow boot stack cleanly.

### 4.2 Scheduler / Interrupts

- **EOI after handler → lost IRQs**  
  Fixed by sending EOI before handler.

- **`IF` never restored after interrupt‑triggered switch**  
  Fixed by enabling interrupts in `thread_trampoline`.

- **Recursive interrupt nesting → stack exhaustion**  
  Temporary fix: increase `KERNEL_STACK_SIZE` to 64 KiB.  
  Proper IST / nesting control is future work.

### 4.3 libc / File Descriptors

- **FD namespace collision (stdin vs first file)**  
  Fixed via `VULCAN_FD_OFFSET` and fd translation layer in libc.

### 4.4 Filesystem / VFS API Gap

- Comment referenced `vfs_readdir` that did not exist.  
- Implemented `vfs_readdir()` in `vfs.c` and `vulcan_readdir()` in libc.

---

## 5. Verification Status

### 5.1 Filesystem Bring‑up

- Self‑test creates `/state/fs-selftest.txt`, writes 57 bytes, reopens, reads, compares.
- Panics on any mismatch.
- Byte count manually verified against VGA font bitmap.

### 5.2 libc Bring‑up

- `libc_selftest()` exercises:
  - String functions
  - `malloc`/`free`
  - `atoi`
  - Full open/write/close/open/read/close round‑trip via libc
- Verified `printf` correctness (`2 + 2 = 4`).
- Stable under 20‑second soak with mm + scheduler + filesystem + libc.

### 5.3 Scheduler

- Verified round‑robin alternation between demo threads.
- Stable under 20‑second soak test with no faults.

### 5.4 Userland

- Verified via QEMU `sendkey`:
  - `help`
  - `ls` (lists all top‑level directories)
  - `cat /state/fs-selftest.txt` (reads self‑test file via full stack)
- Shell idle loop (poll + yield) stable under soak.

---

## 6. High‑Level Roadmap Snapshot

### Short‑Term

- Native bootloader in `boot/*`
- Storage drivers + persistent filesystem
- Syscall boundary and ring‑3 processes
- Per‑process address spaces

### Long‑Term

- Networking stack
- USB subsystem
- Graphical desktop environment
- Installer and package manager
- Expanded userspace ecosystem


