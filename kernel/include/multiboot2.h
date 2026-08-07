/*
 * multiboot2.h - Multiboot2 boot information tag structures
 *
 * GRUB passes a pointer to a tag-based boot information structure in
 * rbx (see long_mode_start in boot.asm, which preserves it into
 * kmain's second argument). This header defines just the tags
 * VulcanOS's mm subsystem actually consumes (memory map, and later
 * framebuffer info once a graphics driver exists) rather than the
 * full Multiboot2 tag set, which includes ELF symbols, ACPI RSDP,
 * boot command line, and others VulcanOS doesn't need yet.
 *
 * Structure layouts match the Multiboot2 specification exactly;
 * field order and sizes here are not a VulcanOS design choice, they
 * are the wire format GRUB actually writes.
 */

#ifndef VULCAN_MULTIBOOT2_H
#define VULCAN_MULTIBOOT2_H

#include "types.h"

#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36D76289

#define MB2_TAG_TYPE_END          0
#define MB2_TAG_TYPE_MMAP         6
#define MB2_TAG_TYPE_FRAMEBUFFER  8

#define MB2_MEMORY_AVAILABLE        1
#define MB2_MEMORY_RESERVED         2
#define MB2_MEMORY_ACPI_RECLAIMABLE 3
#define MB2_MEMORY_NVS              4
#define MB2_MEMORY_BADRAM           5

struct mb2_tag {
    u32 type;
    u32 size;
} __attribute__((packed));

struct mb2_mmap_entry {
    u64 addr;
    u64 len;
    u32 type;
    u32 reserved;
} __attribute__((packed));

struct mb2_tag_mmap {
    u32 type;
    u32 size;
    u32 entry_size;
    u32 entry_version;
    struct mb2_mmap_entry entries[];
} __attribute__((packed));

struct mb2_info_header {
    u32 total_size;
    u32 reserved;
} __attribute__((packed));

/* Walks the tag list starting just after the 8-byte info header at
 * `mb2_info_addr` and calls `visit` once per tag. `visit` returning
 * false stops the walk early (used by callers that only care about
 * one tag type and want to bail once they've found it). */
typedef bool (*mb2_tag_visitor_t)(struct mb2_tag *tag, void *ctx);
void mb2_walk_tags(u64 mb2_info_addr, mb2_tag_visitor_t visit, void *ctx);

#endif /* VULCAN_MULTIBOOT2_H */
