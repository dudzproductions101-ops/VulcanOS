
#ifndef VULCAN_ARCH_GDT_H
#define VULCAN_ARCH_GDT_H

#include "types.h"

#define GDT_SEL_KERNEL_CODE 0x08
#define GDT_SEL_KERNEL_DATA 0x10
#define GDT_SEL_USER_CODE   0x18
#define GDT_SEL_USER_DATA   0x20
#define GDT_SEL_TSS         0x28

struct gdt_entry {
    u16 limit_low;
    u16 base_low;
    u8  base_mid;
    u8  access;
    u8  granularity;
    u8  base_high;
} __attribute__((packed));

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

#endif
