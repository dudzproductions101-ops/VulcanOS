















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





typedef bool (*mb2_tag_visitor_t)(struct mb2_tag *tag, void *ctx);
void mb2_walk_tags(u64 mb2_info_addr, mb2_tag_visitor_t visit, void *ctx);

#endif 
