# VulcanOS

## Version 2.9

<div align="center">
<pre><code>
                 xl""``""lx
                 X8Xxx..xxX8X
                 8X8bdX8bd8X8
                dX8Xbd8XbdX8Xb
               dX8XbdX8XbdX8Xb
              dX8XbdX8X8XbdX8Xb
            .dX8XbdX8X8X8XbdX8Xb.
          .d8X8XbdX8X8X8X8XbdX8X8b.
      _.-dX8X8XbdX8X8X8X8X8XdbX8X8Xb-._
   .-d8X8X8X8bdX8X8X8X8X8X8X8X8db8X8X8X8b-.
.-d8X8X8X8X8bdX8X8X8X8X8X8X8X8X8db8X8X8-RG-b-.
</code></pre>
</div>

VulcanOS is a from-scratch, bare-metal operating system targeting x86_64 systems. The project is focused on building an operating system from the ground up, including the kernel, memory management, filesystem architecture, hardware support, userspace, package management, and eventually a graphical environment.

VulcanOS is the successor to the original OneOS project. Rather than continuing to build on the original codebase, VulcanOS represents a complete rewrite and redesign with a new architecture, identity, and long-term direction.

The project is primarily written in C, with x86 assembly used where low-level hardware interaction requires it. VulcanOS is currently a text-based operating system and does not yet have a graphical desktop environment. The current system operates primarily from RAM, with a RAM-based filesystem and a custom Virtual Filesystem layer providing the foundation for future persistent filesystems.

VulcanOS also includes its own package management system, VPKG, along with a growing collection of kernel and userspace infrastructure. The goal is to gradually turn these foundations into a complete and usable operating system rather than simply creating a bootable kernel.

---

# Current Status

VulcanOS is actively under development. The current focus is on building a stable foundation before moving toward larger features such as persistent storage and graphics.

The operating system currently includes an x86_64 kernel, bare-metal booting, a text-based environment, RAM filesystem support, the VulcanOS Virtual Filesystem (VFS), device filesystem infrastructure, memory management, system-call infrastructure, hardware discovery, process infrastructure, a command-line environment, and the VPKG package system.

There is currently no graphical desktop, persistent disk filesystem, complete networking stack, complete USB stack, or production-ready security model. These are future goals rather than features that are currently complete.

---

# History

VulcanOS began as OneOS, a small terminal-based operating system created by me and my friend Rafa. The original goal was to build an operating system as a learning and resume project, but as development continued, the project grew into a much larger systems programming experiment.

Eventually, the original OneOS architecture reached its limits. Instead of continuing to add features to an increasingly difficult codebase, development moved toward VulcanOS.

VulcanOS is therefore not simply a renamed version of OneOS. It is a substantial rewrite intended to provide a cleaner and more maintainable foundation for future development, with redesigned kernel structures, filesystem architecture, userspace infrastructure, and a new overall direction.

---

# Features

## Text-Based Environment

VulcanOS currently operates through a text-based environment. The system does not yet provide a graphical desktop, window manager, compositor, or graphical application framework.

The current priority is to build and stabilize the underlying operating system before introducing a graphical layer.

## RAM Filesystem

The current filesystem is stored entirely in RAM, meaning its contents are temporary and are lost when the system shuts down or reboots.

This filesystem provides the foundation for the operating system's file handling while persistent storage is being developed.

## Virtual Filesystem

VulcanOS includes its own Virtual Filesystem (VFS), which provides an abstraction between the rest of the operating system and individual filesystem implementations.

This allows the operating system to interact with files and directories through a common interface instead of requiring every component to understand the implementation details of a specific filesystem.

The current filesystem infrastructure includes the VFS, RAM filesystem, device filesystem support, file and directory operations, filesystem abstractions, and device nodes.

Persistent disk-backed filesystems are planned for the future.

## Package System

VulcanOS includes its own package management system called VPKG. The package system is intended to provide a consistent way to package, install, manage, and distribute software for VulcanOS.

As the userspace grows, VPKG will become an increasingly important part of the operating system's software ecosystem.

---

# Built-in Software

VulcanOS currently provides a growing collection of system software and userspace components, including a command-line environment, shell utilities, filesystem utilities, system utilities, package management tools, development tools, and other userspace infrastructure.

The software ecosystem is still in its early stages and will continue to expand as the underlying operating system becomes more capable.

---

# Core System

## Kernel

VulcanOS contains a custom kernel designed specifically for the project. Its current infrastructure includes x86_64 support, CPU initialization, interrupt handling, memory management, paging, system calls, kernel logging, debugging, hardware discovery, and process infrastructure.

The kernel is continuously being expanded as new hardware and operating-system functionality is implemented.

## Memory Management

VulcanOS uses custom memory-management infrastructure for handling physical and virtual memory. Current and planned functionality includes physical memory management, virtual memory, paging, kernel allocation, and userspace memory management.

Memory management is especially important to the current system because the filesystem and much of the operating environment currently operate in RAM.

## Hardware Support

VulcanOS is designed to eventually support a broad range of x86_64 hardware. Current and planned hardware layers include CPU support, ACPI, PCI device discovery, keyboard and mouse input, framebuffer/display support, storage devices, networking hardware, USB devices, and audio devices.

Hardware support is being developed incrementally as the kernel architecture matures.

---

# Process System

VulcanOS is developing its own process and execution infrastructure. This includes process management, threads, scheduling, context switching, inter-process communication, ELF loading, and eventually stronger separation between user and kernel space.

The process system is one of the foundations required for building a larger and more capable userspace environment.

---

# Security

Security is part of the long-term VulcanOS architecture. Planned functionality includes users, groups, file permissions, capability-based security, process isolation, and cryptographic utilities.

VulcanOS is currently an experimental operating system and should not be considered production-ready from a security perspective.

---

# Architecture

VulcanOS uses a modular architecture that separates the boot process, kernel, architecture-specific code, drivers, filesystem subsystem, memory management, process system, system calls, security, userspace, package system, SDK, tests, and development tools.

```text
VulcanOS
│
├── boot/
│   ├── Bootstrap code
│   ├── Entry assembly
│   ├── Bootloader configuration
│   └── Multiboot support
│
├── kernel/
│   ├── arch/
│   │   └── x86_64/
│   │       ├── CPU support
│   │       ├── ACPI
│   │       ├── Interrupts
│   │       ├── Paging
│   │       ├── System calls
│   │       └── Context switching
│   │
│   ├── drivers/
│   ├── fs/
│   ├── memory/
│   ├── process/
│   ├── syscall/
│   ├── security/
│   └── util/
│
├── libc/
│
├── userspace/
│   ├── apps/
│   ├── services/
│   └── shell/
│
├── pkg/
│   └── VPKG
│
├── sdk/
├── tests/
└── tools/
```

The architecture will continue to evolve as new kernel, filesystem, driver, and userspace components are implemented.

---

# Technology Stack

| Component        | Technology                    |
| ---------------- | ----------------------------- |
| Main Language    | C                             |
| Low-level Code   | x86 Assembly                  |
| Architecture     | x86_64                        |
| Boot             | Bootloader / Multiboot        |
| Firmware Support | UEFI groundwork               |
| Graphics         | Planned                       |
| Input            | Hardware drivers              |
| Memory           | Custom memory management      |
| Filesystem       | VulcanOS VFS + RAM filesystem |
| Package System   | VPKG                          |
| Scripting        | PyRT                          |
| Build System     | CMake / Build scripts         |
| Testing          | QEMU                          |

---

# Building and Running

## Requirements

On Debian/Ubuntu-based systems:

```bash
sudo apt update
sudo apt install build-essential gcc g++ cmake grub-pc-bin grub-efi-amd64-bin xorriso mtools qemu-system-x86
```

Additional dependencies may be required depending on the current development branch.

## Build

```bash
chmod +x scripts/build.sh
./scripts/build.sh
```

The build process generates the VulcanOS bootable image.

## Run with QEMU

```bash
qemu-system-x86_64 -cdrom vulcanos.iso -m 512M
```

## Debugging

```bash
qemu-system-x86_64 -cdrom vulcanos.iso -serial stdio
```

QEMU is currently the primary development and testing environment for VulcanOS.

---

# Roadmap

The immediate focus is improving the existing kernel, filesystem, userspace, and package infrastructure. The next major steps include persistent storage, disk-backed filesystems, additional filesystem drivers, improved process isolation, better scheduling, expanded hardware support, networking, USB support, and a more complete userspace.

Graphics are a longer-term goal. The planned graphical system will include framebuffer abstraction, software rendering, a graphics library, window management, a compositor, a desktop environment, a taskbar and launcher, graphical applications, and mouse-driven interaction.

The long-term goal is to turn VulcanOS into a complete standalone operating system with persistent storage, broader hardware support, networking, a mature package ecosystem, a graphical desktop environment, and a growing collection of applications.

---

# Project Philosophy

VulcanOS is primarily an educational and experimental operating system project focused on understanding how operating systems work by implementing major components from scratch.

The project covers kernel development, low-level systems programming, hardware interaction, memory management, filesystem design, process management, package management, userspace development, and operating system architecture.

The goal is not to immediately recreate a modern desktop operating system. Instead, VulcanOS is being built layer by layer, with each part of the system providing a foundation for the next.

---

# Credits

VulcanOS originated from the OneOS project and was developed as a collaborative operating system project with Rafa.

The project is inspired by hobby operating-system development, Unix philosophy, low-level systems programming, and experimental operating-system design.

VulcanOS is built from scratch as a learning, research, and development project.

