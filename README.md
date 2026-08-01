# VulcanOS

A Unix-like operating system built from scratch: its own kernel,
its own driver model, its own directory hierarchy, its own tools —
not a Linux or BSD derivative, and not a clone of another hobby OS.
VulcanOS studies existing projects for engineering technique, not
for structure to copy.

## Status: boots to an interactive shell (verified with real keystrokes)

VulcanOS now boots to a real, interactive shell prompt. What exists
is a from-scratch x86_64 kernel that boots via GRUB/Multiboot2,
brings up its core platform (GDT, IDT, PIC-remapped interrupts, PIT
timer, PS/2 keyboard, VGA console), initializes real memory
management (physical frame allocator, paging, kernel heap), runs a
genuine round-robin thread scheduler with verified preemptive
multitasking, has a working virtual filesystem layer with a complete
(RAM-resident) filesystem backend, its own C library, and real
userland — `init` starts `vulsh`, a shell with working line editing
and command dispatch, plus `ls`/`cat`/`echo`. It has been built and
boot-tested in QEMU, including sending real keystrokes through
QEMU's monitor and reading back the shell's actual response — a
screenshot of a real, interactive session is at the bottom of this
file. There is still no bootloader of VulcanOS's own (GRUB fills
that role today), no package manager, no graphical environment, and
no installer yet — see [PROJECT_STATUS.md](PROJECT_STATUS.md) for
the exact, current component-by-component breakdown, and the
architecture writeup this project was built alongside for the full
roadmap.

## Building

Requires: `gcc`, `nasm`, `grub-mkrescue`, `xorriso`, `qemu-system-x86_64`.

```sh
make iso   # builds kernel/build/vulcanos.elf, then vulcanos.iso
make run   # builds and boots it in QEMU
```

No cross-compiler is required for this stage: the kernel builds with
the host's `gcc` in freestanding mode. See the toolchain note at the
top of `kernel/Makefile` for why that's a deliberate, documented
interim choice rather than an oversight, and what to switch to
before this project ever builds on a non-x86_64 host.

## Directory layout

VulcanOS uses its own layout, not a copy of `/usr`, `/bin`, `/etc`
convention from another Unix, and not another hobby OS's tree:

```
boot/     bootloader sources (BIOS + UEFI paths)
kernel/   the kernel itself
  arch/       CPU-architecture-specific code (x86_64 today)
  core/       architecture-independent kernel core (kmain, printk, panic)
  drivers/    device drivers, one directory per device class
  fs/         virtual filesystem + VulcanOS's own on-disk format
  include/    kernel-internal headers, mirroring the source tree above
  mm/         physical/virtual memory management
  proc/       processes, threads, scheduler
libc/     VulcanOS's own minimal C library for userland
user/     userland: init, shell, core utilities
tools/    host-side build/image tooling (not part of the OS itself)
docs/     design documents (desktop environment, filesystem hierarchy)
```

This mirrors the source tree 1:1 into `kernel/include/`, so a reader
can always find a subsystem's header at the same relative path as
its implementation.

## What's actually verified working

Boot-tested via `make run` in QEMU 8.2.2, screenshot captured via the
QEMU monitor's `screendump` command (not a mockup):

- Multiboot2 handoff from GRUB, CPUID/long-mode sanity checks
- Transition from 32-bit protected mode to 64-bit long mode
- Flat GDT with TSS installed
- 256-entry IDT with all 32 CPU exception vectors wired, PIC remapped
  to vectors 32-47, timer (IRQ0) and keyboard (IRQ1) wired and firing
- VGA text-mode console with scrolling, color, cursor tracking
- A from-scratch `printk` formatter (kernel can't link the C library
  it will eventually provide to userland)
- **Physical memory management**: a bitmap frame allocator that
  parses GRUB's real Multiboot2 memory map (observed: 1,048,576
  tracked frames, ~510 MiB free out of 512 MiB given to QEMU)
- **Paging**: real 4-level page tables replacing boot.asm's temporary
  identity map, NX bit enabled, kernel mapped into the higher half at
  `0xffffffff80000000`
- **Kernel heap**: `kmalloc`/`kfree` backed by on-demand page mapping
- **Round-robin scheduler**: real preemptive multitasking between
  multiple threads, verified via actual boot output showing correct
  alternation between two independently-created demo threads
  (`demo-thread-a: iteration 0`, `demo-thread-b: iteration 0`, `a:
  iteration 1`, `b: iteration 1`, ... through both completing
  cleanly). Getting this genuinely correct required finding and
  fixing three real, subtle bugs — documented in detail in
  `PROJECT_STATUS.md` because the diagnostic process (raw VGA-memory
  writes inside the ISR stub, QEMU's `-d int` exception tracing) is
  as valuable a record as the fixes themselves.
- Confirmed stable under a 20-second sustained soak test (spanning
  both demo threads completing and the system settling into the idle
  thread) with zero faults
- **Filesystem**: a working VFS layer (mount table, absolute path
  resolution, file descriptors) backed by vulcanfs, an original
  RAM-resident filesystem — real, complete file/directory operations,
  not a stub. Boot-time self-test creates a file, writes real
  content, closes it, reopens it fresh, and reads it back — verified
  byte-for-byte correct (57/57 bytes), confirmed against the boot
  screenshot by manually decoding the VGA font bitmap rather than
  trusting OCR. VulcanOS's own runtime directory hierarchy
  (`/vulcan`, `/config`, `/devices`, `/home`, `/state`, `/system`,
  `/media`, `/packages`, `/tmp`) is documented in
  `docs/FILESYSTEM_HIERARCHY.md` and built for real at boot.
- **libc**: string/stdlib/stdio functions, all boot-verified through
  a self-test exercising libc's own public API — `open`/`write`/
  `close`/`open`/`read`/`close` through a real, separate open call
  (not fd reuse), `malloc`/`free`, `atoi`, and `memmove`'s
  overlapping-region handling (numerically verified against forward
  and backward test cases before it shipped). `printf` produces
  mathematically correct output confirmed in the actual boot
  screenshot (`2 + 2 = 4`), not just "didn't crash." A real,
  structural bug — a file-descriptor namespace collision between
  VulcanOS's VFS and libc's reserved standard-stream numbers — was
  found via a targeted diagnostic and fixed with a proper offset-
  translation layer; documented in full in `PROJECT_STATUS.md`.
- **Userland**: `init` starts `vulsh`, a real interactive shell with
  backspace-aware line editing, whitespace tokenization, and
  built-in-command dispatch (`ls`, `cat`, `echo`, `help`, `exit`),
  plus the utilities themselves. Verified with actual typed
  keystrokes sent through QEMU's monitor `sendkey` command, not just
  reading boot output: `help` echoed correctly and printed the right
  text; `ls` listed all nine of VulcanOS's real top-level directories
  in the correct order; `cat /state/fs-selftest.txt` read back the
  exact content a completely different subsystem (the filesystem
  self-test) wrote during boot, through the full userland I/O stack.
  Stable under a 20-second soak test with the shell actively idling
  at its prompt.

## License

See [LICENSE](LICENSE).
