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

VulcanOS is a from-scratch, bare-metal operating system targeting **x86_64 systems**.

VulcanOS is the successor to the original **OneOS** project. It represents a complete rewrite and redesign of the original system, with a new architecture, identity, and long-term direction.

The project is primarily written in **C**, with **x86 assembly** used where low-level hardware interaction requires it.

VulcanOS is currently a **text-based operating system**. It does **not yet have a graphical desktop environment**.

The current system is also **RAM-based**: the active filesystem exists in memory and is not yet backed by persistent storage.

The long-term goal is to build a complete operating system from the ground up, including persistent storage, hardware drivers, userspace applications, networking, and eventually a graphical environment.

---

# Current Status

VulcanOS is actively under development.

### Currently implemented

* x86_64 kernel
* Bare-metal booting
* Text-based system environment
* RAM-based filesystem
* VulcanOS Virtual Filesystem (VFS)
* Device filesystem support
* Process and kernel infrastructure
* System calls
* Memory management
* Hardware discovery
* Package management system
* Command-line environment
* Basic userspace infrastructure

### Not implemented yet

* Graphical desktop environment
* Persistent filesystem/storage
* Full disk filesystem drivers
* Complete networking stack
* Complete USB stack
* Full hardware support
* Production-ready security model

The graphical system and persistent filesystem are part of the long-term roadmap and should not be considered current features.

---

# History

VulcanOS originally started as **OneOS**, a small terminal-only operating system created by me and my friend Rafa.

The original goal was to build an operating system as both a learning project and a resume project. As development progressed, the project grew into a much larger systems programming effort.

OneOS eventually reached the limits of its original architecture. Rather than continuing to extend the existing system indefinitely, development transitioned into VulcanOS.

VulcanOS is not simply a renamed version of OneOS. It is a substantial rewrite with:

* A new architecture
* A new identity
* A redesigned kernel structure
* A new filesystem architecture
* New userspace infrastructure
* A new long-term development direction

The goal is to create a cleaner, more maintainable foundation for future operating system development.

---

# Features

## Text-Based Environment

VulcanOS currently operates entirely through a **text-based environment**.

The operating system does not currently provide a graphical desktop, window manager, compositor, or graphical application framework.

The current focus is on building and stabilizing the underlying operating system before introducing a graphical layer.

---

## RAM-Based Filesystem

The current VulcanOS filesystem is stored entirely in **RAM**.

This means that filesystem contents are temporary and are lost when the system shuts down or reboots.

The RAM filesystem provides the foundation for the operating system's filesystem architecture while persistent storage support is being developed.

---

## VulcanOS Virtual Filesystem

VulcanOS includes its own **Virtual Filesystem (VFS)** layer, referred to as the **Vulcan Filesystem architecture**.

The VFS provides a common interface between userspace/kernel components and filesystem implementations.

The current architecture is intended to eventually support multiple filesystem backends, including persistent storage filesystems.

Current filesystem infrastructure includes:

* Virtual filesystem layer
* RAM filesystem
* Device filesystem support
* File and directory operations
* Filesystem abstractions
* Device nodes

Persistent disk-backed filesystems are planned for future development.

---

## Package System

VulcanOS already includes a package management system.

The package system is designed to provide a way to install, manage, and distribute software for the operating system.

The current package system is called **VPKG**.

VPKG is part of the VulcanOS software ecosystem and is intended to become the primary way of managing userspace software and system packages.

---

# Built-in Software

VulcanOS currently provides a growing collection of system software and userspace components.

Current and developing components include:

* Command-line environment
* Shell utilities
* Filesystem utilities
* System utilities
* Package management tools
* Development tools
* Userspace infrastructure

Additional applications will be added as the userspace environment matures.

A graphical application ecosystem is planned, but is **not currently implemented**.

---

# Booting

VulcanOS currently boots using a bootloader-based x86_64 boot process and can be tested using virtualization.

The project is primarily developed and tested using **QEMU**.

Real hardware support is an ongoing area of development and depends on the maturity of the kernel and hardware drivers.

---

# Core System

## Kernel

VulcanOS contains a custom kernel designed specifically for the project.

Current kernel infrastructure includes:

* x86_64 architecture support
* CPU initialization
* Interrupt handling
* Memory management
* Paging infrastructure
* System calls
* Kernel logging
* Kernel debugging
* Hardware discovery
* Process infrastructure

---

## Memory Management

VulcanOS uses custom memory-management infrastructure designed for its kernel environment.

Current and planned functionality includes:

* Physical memory management
* Virtual memory
* Paging
* Kernel allocation
* Userspace memory management

Memory management is a core component of the current development effort because the operating system and filesystem currently operate primarily in RAM.

---

## Hardware Support

VulcanOS is designed to eventually support a broad range of x86_64 hardware.

Current and planned hardware layers include:

* CPU support
* ACPI
* PCI device discovery
* Keyboard input
* Mouse input
* Display/framebuffer support
* Storage devices
* Network devices
* USB devices
* Audio devices

Hardware support is being developed incrementally as the kernel architecture matures.

---

# Process System

VulcanOS is developing its own process and execution infrastructure.

Current and planned functionality includes:

* Process management
* Threads
* Scheduling
* Context switching
* Inter-process communication
* ELF loading
* User/kernel separation

---

# Security

Security infrastructure is part of the long-term VulcanOS architecture.

Planned functionality includes:

* Users
* Groups
* File permissions
* Capability-based security
* Process isolation
* Cryptographic utilities

The current system should be considered experimental and is **not production-ready from a security perspective**.

---

# Architecture

VulcanOS uses a modular operating system architecture.

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
│   │   ├── Graphics
│   │   ├── Storage
│   │   ├── Network
│   │   ├── Audio
│   │   ├── Input
│   │   └── USB
│   │
│   ├── fs/
│   │   ├── VFS
│   │   ├── RAM filesystem
│   │   └── Device filesystem
│   │
│   ├── memory/
│   │   └── Memory management
│   │
│   ├── process/
│   │   └── Process and thread system
│   │
│   ├── syscall/
│   │   └── Kernel interface
│   │
│   ├── security/
│   │   └── Security subsystem
│   │
│   └── util/
│       └── Kernel utilities
│
├── libc/
│   └── Minimal C library
│
├── userspace/
│   ├── apps/
│   │   └── System applications
│   │
│   ├── services/
│   │   └── System services
│   │
│   └── shell/
│       └── Command-line environment
│
├── pkg/
│   └── VPKG package system
│
├── sdk/
├── tests/
└── tools/
```

The architecture will evolve as additional kernel, filesystem, driver, and userspace components are implemented.

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

---

## Build

```bash
chmod +x scripts/build.sh
./scripts/build.sh
```

The build process generates the VulcanOS bootable image.

---

## Run with QEMU

```bash
qemu-system-x86_64 -cdrom vulcanos.iso -m 512M
```

---

## Debugging

```bash
qemu-system-x86_64 -cdrom vulcanos.iso -serial stdio
```

QEMU is the primary development and testing environment for VulcanOS.

---

# Roadmap

## Filesystem

* [x] RAM filesystem
* [x] VulcanOS VFS
* [x] Device filesystem infrastructure
* [x] Package system
* [ ] Persistent storage
* [ ] Disk-backed filesystem
* [ ] Filesystem persistence across reboots
* [ ] Additional filesystem drivers

---

## Kernel

* [x] x86_64 kernel foundation
* [x] Memory-management infrastructure
* [x] Interrupt infrastructure
* [x] System-call infrastructure
* [ ] Improve process isolation
* [ ] Improve scheduling
* [ ] Expand hardware support
* [ ] Improve kernel stability

---

## Userspace

* [x] Text-based environment
* [x] Shell infrastructure
* [x] Package management
* [ ] Expand system utilities
* [ ] Expand userspace APIs
* [ ] Improve developer tooling
* [ ] More complete userspace environment

---

## Graphics

The graphical environment is **not implemented yet**.

Planned work includes:

* [ ] Framebuffer abstraction
* [ ] Software rendering
* [ ] Graphics library
* [ ] Window management
* [ ] Compositor
* [ ] Desktop environment
* [ ] Taskbar/launcher
* [ ] Graphical applications
* [ ] Mouse-driven graphical interaction

The graphical desktop will be built on top of the existing kernel, driver, filesystem, and userspace foundations rather than being treated as the current core of the project.

---

## Long-Term Goals

* Persistent storage
* More complete hardware support
* Networking
* USB support
* Improved security
* Better developer tools
* Expanded package ecosystem
* Graphical desktop environment
* Graphical applications
* A usable standalone operating system

---

# Project Philosophy

VulcanOS is primarily an **educational and experimental operating system project**.

The project focuses on understanding how operating systems work by implementing major components from scratch rather than relying on existing operating-system infrastructure.

Areas of focus include:

* Kernel development
* Low-level systems programming
* Hardware interaction
* Memory management
* Filesystem design
* Process management
* Package management
* Userspace development
* Operating system architecture

The goal is not to immediately reproduce a modern desktop operating system. The goal is to build the foundations step by step and understand each layer of the system along the way.

---

# Credits

VulcanOS originated from the **OneOS** project and was developed as a collaborative operating system project with **Rafa**.

VulcanOS is inspired by hobby operating-system development, Unix philosophy, low-level systems programming, and experimental operating-system design.

The project is built from scratch as a learning, research, and development project.
