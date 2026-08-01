# VulcanOS Architecture

A precise, implementation‑verified architectural specification describing how VulcanOS is structured internally, based strictly on the current real codebase.

---

## 1. System Overview

VulcanOS is a **monolithic, higher‑half x86_64 operating system** built from scratch.  
Its architecture is intentionally modular, with strict layering:

- arch → mm → proc → fs → libc → userland

The system currently boots via **GRUB + Multiboot2**, initializes CPU state, memory management, interrupts, scheduling, filesystem, libc, and launches userland programs as kernel threads.

---

## 2. Boot & Early Initialization

### 2.1 Bootloader
- **Current:** GRUB (Multiboot2)
- **Future:** Native VulcanOS bootloader (`boot/*`)

### 2.2 CPU Bring‑up
- CPUID checks  
- Long‑mode transition  
- Temporary identity paging  
- Higher‑half jump

### 2.3 Descriptor Tables
- **GDT + TSS**  
- **IDT (256 entries)**  
- **ISR/IRQ stubs** for exceptions + timer + keyboard

### 2.4 Interrupt Controller
- PIC remap (vectors 32–47)  
- IRQ dispatch + handler registration  
- Correct EOI ordering (verified)

---

## 3. Memory Architecture

### 3.1 Physical Memory Manager
- Bitmap allocator  
- Parses Multiboot2 memory map  
- Reserves kernel footprint + first 1 MiB

### 3.2 Paging / Virtual Memory
- 4‑level page tables  
- NX bit  
- Higher‑half kernel mapping (`VULCAN_KERNEL_VBASE`)  
- On‑demand page‑table allocation

### 3.3 Kernel Heap
- First‑fit free‑list allocator  
- Grows via `paging_map_page`  
- Backing store for vulcanfs + libc + kernel structures

---

## 4. Process & Thread Model

### 4.1 Threads
- Per‑thread kernel stacks  
- Callee‑saved context switch  
- First‑run trampoline (`thread_trampoline`)  
- Verified round‑robin scheduling

### 4.2 Processes
- PCB with PID + thread list  
- **All processes share kernel address space**  
- No ring‑3 yet (future work)

### 4.3 Scheduler
- Round‑robin  
- 5‑tick time slices  
- Preemption via PIT timer IRQ  
- Verified under soak tests

### 4.4 Interrupt‑Related Fixes
- Correct EOI ordering  
- Correct `IF` restoration  
- Temporary fix for interrupt‑nesting stack exhaustion (64 KiB stacks)

---

## 5. Filesystem Architecture

### 5.1 VFS Layer
- Mount table (longest‑prefix match)  
- Absolute path resolution  
- Open‑file descriptor table  
- Backend‑agnostic `inode_ops` vtable

### 5.2 vulcanfs (RAM‑resident)
- Full file + directory operations  
- In‑memory tree  
- No persistence (by design)  
- Clean separation from VFS via vtable

### 5.3 Filesystem Hierarchy
Top‑level directories:
`/vulcan`, `/config`, `/devices`, `/home`, `/state`, `/system`, `/media`, `/packages`, `/tmp`

### 5.4 Filesystem Self‑Test
- Create file  
- Write 57 bytes  
- Close → reopen → read → compare  
- Panics on mismatch

---

## 6. libc Architecture

### 6.1 Direct Kernel Calls (No Syscalls Yet)
- `malloc` → `kmalloc`  
- `open` → `vfs_open`  
- `write` → `vfs_write`  
- `exit` → scheduler termination

### 6.2 Implemented Components
- String functions  
- Memory functions  
- Process primitives  
- Full `printf`  
- `vulcan_readdir` wrapper

### 6.3 FD Namespace Fix
- VFS returns fds starting at 0  
- libc reserves 0/1/2 for stdin/stdout/stderr  
- Introduced `VULCAN_FD_OFFSET = 3`  
- libc translates fds before calling VFS

### 6.4 libc Self‑Test
- String tests  
- `malloc`/`free`  
- `atoi`  
- Full open/write/close/open/read/close round‑trip  
- Verified stable under soak test

---

## 7. Userland Architecture

### 7.1 Execution Model
- No ring‑3  
- No `exec()`  
- Userland programs are compiled into kernel  
- Launched as threads

### 7.2 init
- PID 2  
- Adapts `void(void)` → `argc`/`argv`  
- Launches `vulsh`

### 7.3 Shell (vulsh)
- Interactive line editing  
- Backspace‑aware  
- Tokenization  
- Built‑in command dispatch (direct function calls)

### 7.4 Core Utilities
- `ls` (via `vulcan_readdir`)  
- `cat` (via libc I/O)  
- `echo`

### 7.5 Interactive Verification
- Real keystrokes via QEMU `sendkey`  
- Verified `help`, `ls`, `cat /state/fs-selftest.txt`  
- Shell idle loop stable under soak test

---

## 8. Drivers & Console

### VGA Console
- 16‑color text mode  
- Scrolling  
- Hardware cursor

### PIT Timer
- 100 Hz tick  
- Drives scheduler

### PS/2 Keyboard
- Scancode set 1  
- Shift state  
- Ring buffer

---

## 9. Build & Verification

### Build System
- Freestanding GCC  
- `Makefile` + `grub-mkrescue`  
- Deterministic ISO builds

### Verification Commands
```sh
wc -c <path>
make clean && make iso
make run
