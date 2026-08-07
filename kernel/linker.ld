/*
 * linker.ld - VulcanOS kernel linker script
 *
 * Loads the kernel at the conventional 1 MiB physical mark (below
 * this is legacy BIOS/real-mode territory that GRUB and firmware
 * reserve). Sections are grouped and page-aligned so that, once the
 * mm subsystem takes over from boot.asm's temporary identity
 * mapping, each segment can be mapped with the correct permissions
 * (.text executable+read-only, .rodata read-only, .data/.bss
 * writable+no-execute) rather than one blanket RWX region.
 */

ENTRY(_start)

SECTIONS
{
    . = 1M;

    kernel_start = .;

    .multiboot2 : ALIGN(8)
    {
        KEEP(*(.multiboot2))
    }

    .text : ALIGN(4K)
    {
        *(.text)
        *(.text.*)
    }

    .rodata : ALIGN(4K)
    {
        *(.rodata)
        *(.rodata.*)
    }

    .data : ALIGN(4K)
    {
        *(.data)
        *(.data.*)
    }

    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }

    kernel_end = .;

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
        *(.note.*)
    }
}
