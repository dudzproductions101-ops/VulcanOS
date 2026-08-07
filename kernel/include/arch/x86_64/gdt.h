/*
 * gdt.h - Global Descriptor Table
 *
 * Long mode barely uses segmentation, but x86_64 still requires a
 * valid GDT with correct code/data descriptors and a TSS descriptor
 * (needed later for privilege-level stack switches on interrupt
 * entry). VulcanOS installs a minimal flat GDT during early boot.
 */

#ifndef VULCAN_ARCH_GDT_H
#define VULCAN_ARCH_GDT_H

#include "types.h"

/* Selector values, matching the layout gdt_install() builds.
 * Kept as named constants so callers never hardcode "0x08". */
#define GDT_SEL_KERNEL_CODE 0x08
#define GDT_SEL_KERNEL_DATA 0x10
#define GDT_SEL_USER_CODE   0x18
#define GDT_SEL_USER_DATA   0x20
#define GDT_SEL_TSS         0x28

/* One 8-byte GDT entry, packed to match the exact hardware layout. */
struct gdt_entry {
    u16 limit_low;
    u16 base_low;
    u8  base_mid;
    u8  access;
    u8  granularity;
    u8  base_high;
} __attribute__((packed));

/* The TSS descriptor is 16 bytes in long mode (it needs to hold a
 * 64-bit base address), unlike ordinary segment descriptors. */
struct tss_entry_low {
    u16 length;
    u16 base_low;
    u8  base_mid;
    u8  flags1;
    u8  flags2;
    u8  base_high;
} __attribute__((packed));

struct tss_entry_high {
    u32 base_upper;
    u32 reserved;
} __attribute__((packed));

struct gdt_ptr {
    u16 limit;
    u64 base;
} __attribute__((packed));

/* Minimal 64-bit Task State Segment. VulcanOS does not yet use
 * hardware task switching (long mode doesn't support it), but every
 * interrupt handler needs a valid TSS to source its stack pointer
 * from when switching privilege levels, so the structure is defined
 * now even though rsp0 is only wired up once user mode exists. */
struct tss {
    u32 reserved0;
    u64 rsp0;
    u64 rsp1;
    u64 rsp2;
    u64 reserved1;
    u64 ist[7];
    u64 reserved2;
    u16 reserved3;
    u16 iomap_base;
} __attribute__((packed));

void gdt_install(void);

#endif /* VULCAN_ARCH_GDT_H */
