# VulcanOS

## Version 2.9

<div align="center">
  <pre><code>
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
  </code></pre>
</div>

VulcanOS is a from-scratch, bare-metal graphical operating system designed for **x86_64 systems**.

VulcanOS is the successor to the original **OneOS** project, representing a complete rewrite, redesign, and identity change. Instead of continuing the previous architecture, VulcanOS introduces a new foundation focused on:

- Cleaner system design
- Better separation between components
- A scalable operating system structure

The project aims to create a complete computing environment from the ground up, including:

- Custom kernel
- Drivers
- Userspace
- Graphical desktop environment
- System utilities

Written primarily in **C**, with low-level **assembly** used where required, VulcanOS is an educational showcase project focused on:

- Operating system development
- Kernel engineering
- Hardware interaction
- Graphics programming
- Low-level systems design

---

# History

VulcanOS originally started as **OneOS**, a small terminal-only operating system created by me and my friend Rafa.

The original goal was simple: build an operating system as a learning project and resume project. However, as development continued, the project grew far beyond its original scope.

What started as an experiment became a larger systems programming project involving:

- Kernel development
- Hardware support
- Memory management
- Graphics systems
- Custom applications
- Operating system architecture

After reaching the limits of the original design, the project transitioned into VulcanOS.

This transition was not a simple update. VulcanOS is a complete rewrite featuring:

- A new architecture
- A new identity
- A redesigned kernel structure
- A new long-term direction

The goal is to build a cleaner and more maintainable operating system foundation.

---

# Features

## Graphical Desktop Environment

VulcanOS includes a custom graphical environment built from scratch.

Current and planned features:

- Software-rendered graphical interface
- Custom compositor
- Window management
- Desktop environment
- Taskbar and launcher system
- Application windows
- Custom rendering pipeline
- Mouse and keyboard interaction

The graphical system is designed without depending on existing desktop environments or graphical frameworks.

Current progress includes early graphics support from GRUB's Multiboot2 framebuffer tag, a software-rendered boot splash, and a `/devices/fb0` device node for userland display discovery.

---

## Built-in Applications

VulcanOS includes a growing userspace environment.

Applications include:

- Terminal emulator
- File manager
- Text editor
- Calculator
- Settings application
- Desktop environment components
- System utilities

Future applications will expand the VulcanOS software ecosystem.

---

## Booting on Real Hardware

VulcanOS currently boots via a GRUB multiboot2 ISO. The top-level `Makefile` now supports building a hybrid USB image with:

- `make usb`

The resulting `vulcanos-usb.img` can be written directly to a USB stick with `dd` on Linux, or a similar raw-image tool on other platforms. This is the practical path to boot VulcanOS on real x86_64 hardware that supports BIOS/UEFI legacy boot.

---

# Core System Features

## Kernel

- Custom kernel architecture
- x86_64 support
- Interrupt handling
- CPU initialization
- ACPI support
- Paging and memory management
- System call infrastructure
- Kernel logging and debugging

---

## Hardware Support

Current and planned hardware layers:

- Framebuffer graphics
- PS/2 keyboard and mouse
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

---

## Process System

Planned process features:

- Process management
- Threading
- Scheduling
- IPC
- ELF loading
- User/kernel separation

---

## Security

Planned security systems:

- Users
- Permissions
- Groups
- Capability systems
- Cryptographic utilities

---

# Architecture

VulcanOS uses a modular operating system architecture.

```text
VulcanOS
│
├── boot/
│   ├── Bootstrap code
│   ├── Entry assembly
│   ├── Limine configuration
│   ├── Multiboot support
│   └── UEFI initialization
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
│   │   └── Filesystem subsystem
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
│   ├── desktop/
│   │   ├── Compositor
│   │   ├── Taskbar
│   │   ├── Launcher
│   │   └── Desktop components
│   │
│   ├── apps/
│   │   ├── Terminal
│   │   ├── File manager
│   │   ├── Calculator
│   │   ├── Settings
│   │   └── Text editor
│   │
│   ├── services/
│   │   └── System services
│   │
│   └── shell/
│       └── Command-line environment
│
├── sdk/
├── tests/
└── tools/
```
# Technology Stack

| Component | Technology |
|---|---|
| Main Language | C |
| Low-level Code | x86 Assembly |
| Architecture | x86_64 |
| Boot | Limine / Multiboot |
| Firmware Support | UEFI groundwork |
| Graphics | Custom software rendering |
| Input | PS/2 and hardware drivers |
| Memory | Custom memory management |
| Scripting | PyRT |
| Package System | VPKG |
| Build System | CMake / Build scripts |
| Testing | QEMU |
# Building and Running

## Requirements

Install the required tools:

```bash
sudo apt update
sudo apt install build-essential gcc g++ cmake grub-pc-bin grub-efi-amd64-bin xorriso mtools qemu-system-x86
```

---

## Build

```bash
chmod +x scripts/build.sh
./scripts/build.sh
```

The build process will generate the VulcanOS image.

---

## Run

```bash
qemu-system-x86_64 -cdrom vulcanos.iso -m 512M
```

---

## Debugging

```bash
qemu-system-x86_64 -cdrom vulcanos.iso -serial stdio
```

---

# Roadmap

## Current Goals

- Finish VulcanOS rewrite
- Stabilize kernel systems
- Improve desktop environment
- Fix existing bugs
- Improve hardware compatibility
- Expand userspace applications
- Improve system usability

---

## Future Goals

- Complete filesystem support
- More hardware drivers
- Networking support
- Improved security model
- Better developer tools
- More complete desktop experience
- Move toward a usable standalone operating system

---

# Credits

VulcanOS is an educational operating system project focused on:

- Low-level systems programming
- Kernel development
- Building a complete environment from scratch

Inspired by hobby operating system development, Unix philosophy, and experimental system design.
