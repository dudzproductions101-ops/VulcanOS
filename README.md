# VulcanOS

## Version 2.9

<div align="center">

```text
                     `'``''`.
                     .'.``'`
                  .'`..'''`.
                    ````.'`
                  xl""``""lx
                 X8Xxx..xxX8X
                 8X8bdX8bd8X8
                dX8Xbd8XbdX8Xb
               dX8Xbd8X8XbdX8Xb
              dX8Xbd8X8X8XbdX8Xb
            .dX8Xbd8X8X8X8XbdX8Xb.
          .d8X8Xbd8X8X8X8X8XbdX8X8b.
      _.-dX8X8Xbd8X8X8X8X8X8XdbX8X8Xb-._
   .-d8X8X8X8bdX8X8X8X8X8X8X8X8db8X8X8X8b-.
.-d8X8X8X8X8bdX8X8X8X8X8X8X8X8X8db8X8X8-RG-b-.
```

</div>

---

## Overview

VulcanOS is a from-scratch, bare-metal graphical operating system designed for x86_64 systems.

It is the successor to the original OneOS project and represents a complete rewrite with a new architecture, improved system design, and a long-term focus on building a complete computing environment.

VulcanOS is built from the ground up with:

- Custom kernel
- Hardware drivers
- Userspace environment
- Graphical desktop system
- System utilities
- Developer tools

The project is primarily written in C with low-level x86_64 assembly where required.

VulcanOS focuses on:

- Operating system development
- Kernel engineering
- Hardware interaction
- Graphics programming
- Memory management
- Low-level systems design

---

# History

VulcanOS started as **OneOS**, a small terminal-based operating system created as a learning project with my friend Rafa.

The original goal was to create an operating system from scratch for education and experience.

Over time, the project expanded into a larger systems programming project involving:

- Kernel development
- Hardware support
- Memory management
- Graphics systems
- Custom applications
- Operating system architecture

After reaching the limits of the original design, OneOS became VulcanOS.

VulcanOS introduced:

- New architecture
- New identity
- Redesigned kernel structure
- New development direction

The goal is to create a cleaner and more maintainable operating system foundation.

---

# Features

## Kernel

Current and planned kernel features:

- x86_64 architecture support
- CPU initialization
- Interrupt handling
- ACPI support
- Paging and memory management
- Kernel heap allocator
- Process system
- Thread scheduling
- System call infrastructure
- Kernel logging and debugging

---

## Graphics System

VulcanOS includes a custom graphical environment built without existing desktop frameworks.

Features include:

- Software rendering
- Custom compositor
- Window management
- Desktop environment
- Taskbar and launcher system
- Application windows
- Mouse and keyboard input

---

## Hardware Support

Supported and planned hardware layers:

- Framebuffer graphics
- PS/2 keyboard
- PS/2 mouse
- PCI device discovery
- Storage drivers
- Network drivers
- Audio drivers
- USB support

---

## Filesystem

Filesystem architecture includes:

- Virtual filesystem layer
- Device filesystem support
- Temporary filesystem support
- Filesystem drivers
- VulcanFS memory filesystem

---

## Userspace

VulcanOS includes a growing userspace environment.

Current and planned applications:

- Terminal
- Shell
- File manager
- Text editor
- Calculator
- Settings application
- System utilities

---

# Architecture

VulcanOS uses a modular operating system architecture.

```text
VulcanOS
│
├── boot/
│   ├── Boot configuration
│   ├── Entry code
│   └── Firmware initialization
│
├── kernel/
│   ├── arch/
│   │   └── x86_64/
│   │       ├── CPU support
│   │       ├── Interrupts
│   │       ├── Paging
│   │       └── System calls
│   │
│   ├── drivers/
│   │   ├── Graphics
│   │   ├── Storage
│   │   ├── Network
│   │   ├── Audio
│   │   └── Input
│   │
│   ├── fs/
│   │   └── Filesystem subsystem
│   │
│   ├── memory/
│   │   └── Memory management
│   │
│   ├── process/
│   │   └── Scheduler and processes
│   │
│   └── security/
│       └── Security systems
│
├── libc/
│   └── Minimal C library
│
├── userspace/
│   ├── desktop/
│   ├── apps/
│   ├── services/
│   └── shell/
│
├── sdk/
├── tests/
└── tools/
```

---

# Technology Stack

| Component | Technology |
|---|---|
| Main Language | C |
| Low Level Code | x86 Assembly |
| Architecture | x86_64 |
| Boot System | Limine / Multiboot |
| Firmware | UEFI groundwork |
| Graphics | Custom software rendering |
| Input | PS/2 and hardware drivers |
| Memory | Custom memory management |
| Build System | CMake / Make |
| Testing | QEMU |

---

# Building

## Requirements

Install required packages:

```bash
sudo apt update

sudo apt install \
build-essential \
gcc \
g++ \
cmake \
grub-pc-bin \
grub-efi-amd64-bin \
xorriso \
mtools \
qemu-system-x86
```

---

## Build

```bash
chmod +x scripts/build.sh
./scripts/build.sh
```

The build process generates a bootable VulcanOS image.

---

# Running

Run VulcanOS with QEMU:

```bash
qemu-system-x86_64 \
-cdrom vulcanos.iso \
-m 512M
```

Debug mode:

```bash
qemu-system-x86_64 \
-cdrom vulcanos.iso \
-serial stdio
```

---

# Roadmap

## Current Goals

- Finish VulcanOS rewrite
- Improve kernel stability
- Expand desktop environment
- Improve hardware compatibility
- Expand userspace applications
- Improve system usability

## Future Goals

- Persistent filesystem
- More hardware drivers
- Networking support
- Improved security model
- Developer tools
- Complete desktop experience
- Move toward a usable standalone operating system

---

# Project Status

For detailed implementation status, completed systems, debugging notes, and development progress see:

```
docs/STATUS.md
```

---

# Documentation

Additional documentation:

```
docs/
├── ARCHITECTURE.md
├── STATUS.md
├── DEVELOPMENT.md
├── ROADMAP.md
└── DEBUGGING.md
```

---

# Credits

VulcanOS is an educational operating system project focused on:

- Kernel development
- Systems programming
- Hardware interaction
- Building a complete environment from scratch

Inspired by hobby operating system development, Unix philosophy, and experimental system design.
