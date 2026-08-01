# VulcanOS Boot Architecture

A precise, implementation‑verified description of how VulcanOS boots today, what exists, and what is intentionally missing.
# 1. Current Boot Flow (GRUB + Multiboot2)

VulcanOS currently boots using GRUB and the Multiboot2 specification.
A native bootloader does not exist yet.
## 1.1 GRUB Responsibilities

    Loads the kernel ELF into memory

    Provides:

        Multiboot2 magic

        Multiboot2 info structure

        Memory map

        Framebuffer info (if enabled)

    Transfers control to the kernel’s Multiboot2 entry point

## 1.2 Kernel Entry (boot.asm)

The first executed VulcanOS code:

    Validates Multiboot2 magic

    Saves magic + info pointer into .bss (fix for 32→64‑bit mismatch)

    Performs CPUID vendor + long‑mode checks

    Enables PAE and long mode

    Sets up temporary identity paging

    Switches to 64‑bit mode

    Jumps to higher‑half kmain

# 2. Early Kernel Bring‑up
## 2.1 CPU Initialization

    CPUID feature detection

    CR0/CR4 setup

    EFER.LME + EFER.NXE

    Temporary identity map

    Higher‑half transition

## 2.2 Descriptor Tables

    GDT + TSS installed

    IDT with 256 entries

    ISR/IRQ stubs for exceptions, timer, keyboard

    PIC remapped to vectors 32–47

## 2.3 Memory Initialization

    Multiboot2 tag parsing

    Physical memory manager initialization

    Paging initialization (4‑level, NX, higher‑half)

    Kernel heap initialization

# 3. Boot‑Time Self‑Tests
## 3.1 Filesystem Self‑Test

    Create /state/fs-selftest.txt

    Write 57 bytes

    Close → reopen → read → compare

    Panic on mismatch

## 3.2 libc Self‑Test

    Test string functions

    Test malloc and free

    Test atoi

    Full open/write/close/open/read/close round‑trip

    Verify printf correctness

    Confirm scheduler stability

## 3.3 Scheduler Verification

    Timer IRQ fires repeatedly

    Round‑robin alternation between demo threads

    Verified via QEMU interrupt tracing and VGA diagnostics

## 4. Boot Bugs Fixed
4.1 Register Corruption Across Handoff

Cause: 32‑bit pushes popped as 64‑bit values after nested calls.
Fix: Store magic + info pointer in .bss and reload in long mode.
4.2 NASM .bss Alignment Warning Storm

Cause: Small reservation before align 4096 produced thousands of warnings.
Fix: Reordered .bss so page tables follow the boot stack cleanly.
## 5. Native Bootloader (Future Work)

The boot/ directory is intentionally empty.
A real VulcanOS bootloader will replace GRUB.
5.1 BIOS Path (Legacy)

# Planned features:

    Real‑mode → protected‑mode → long‑mode transition

    Disk reading (CHS/LBA)

    ELF loading

    E820 memory map retrieval

    VBE framebuffer setup

    Custom boot protocol

## 5.2 UEFI Path (Modern)

Planned features:

    PE/COFF loading

    GOP framebuffer setup

    Memory map handoff

    ExitBootServices

    Unified boot protocol

## 5.3 Unified Boot Protocol (VulcanBoot)

Will provide:

    Kernel physical and virtual base

    Memory map

    Framebuffer info

    ACPI RSDP

    SMP topology

    Command line

    Optional modules (initrd)

## 6. Boot Roadmap
Short‑Term

    BIOS stage‑1 and stage‑2 loader

    UEFI loader

    VulcanBoot protocol

    Remove GRUB dependency

Mid‑Term

    ACPI handoff

    SMP initialization

    Module loading (initrd)

Long‑Term

    Secure boot

    Boot‑time diagnostics

    Graphical boot splash

## 7. Summary

    Current: GRUB + Multiboot2 booting, fully working 64‑bit bring‑up.

    Missing: Native bootloader (boot/* empty by design).

    Future: Unified BIOS/UEFI loader with VulcanBoot protocol.
