/*
 * pmm.c - Physical Memory Manager (bitmap frame allocator)
 *
 * BOOTSTRAP NOTE: the bitmap itself cannot be allocated by the
 * allocator it implements (nothing exists yet to allocate it from).
 * It is instead a fixed-size static array in .bss, sized for the
 * largest physical memory VulcanOS's first bring-up milestone
 * targets (see MAX_TRACKED_FRAMES below). This is a real, documented
 * limitation, not an oversight: systems with more physical memory
 * than MAX_TRACKED_FRAMES covers will have the excess silently
 * unusable until this is replaced with a dynamically-sized bitmap
 * placed after the kernel image once paging can map arbitrary
 * physical pages (a chicken-and-egg problem paging.c's early-boot
 * identity map exists specifically to break).
 */

#include "mm/pmm.h"
#include "multiboot2.h"
#include "printk.h"
#include "panic.h"

/* 4 GiB of tracked physical memory at 4 KiB/frame = 1,048,576 frames
 * = 131,072 bytes of bitmap (128 KiB). Chosen as a reasonable cap
 * for a bring-up milestone kernel; see the bootstrap note above for
 * what a proper fix looks like. */
#define MAX_TRACKED_FRAMES (4ULL * 1024 * 1024 * 1024 / PMM_FRAME_SIZE)
#define BITMAP_SIZE_BYTES (MAX_TRACKED_FRAMES / 8)

static u8 frame_bitmap[BITMAP_SIZE_BYTES];
static u64 total_frames = 0;
static u64 free_frames_count = 0;
static u64 highest_frame_seen = 0;

/* Provided by linker.ld; mark the kernel's own physical footprint
 * as reserved so the allocator never hands out a frame the kernel
 * itself is sitting in. */
extern u8 kernel_start[];
extern u8 kernel_end[];

static inline void bitmap_set(u64 frame)
{
    frame_bitmap[frame / 8] |= (u8)(1 << (frame % 8));
}

static inline void bitmap_clear(u64 frame)
{
    frame_bitmap[frame / 8] &= (u8)~(1 << (frame % 8));
}

static inline bool bitmap_test(u64 frame)
{
    return (frame_bitmap[frame / 8] & (1 << (frame % 8))) != 0;
}

static void reserve_region(paddr_t start, paddr_t end)
{
    u64 first_frame = start / PMM_FRAME_SIZE;
    u64 last_frame = (end + PMM_FRAME_SIZE - 1) / PMM_FRAME_SIZE;

    for (u64 f = first_frame; f < last_frame && f < MAX_TRACKED_FRAMES; f++) {
        if (!bitmap_test(f)) {
            bitmap_set(f);
            if (free_frames_count > 0) {
                free_frames_count--;
            }
        }
    }
}

struct mmap_visit_ctx {
    bool found;
};

static bool mmap_tag_visitor(struct mb2_tag *tag, void *ctx_ptr)
{
    struct mmap_visit_ctx *ctx = (struct mmap_visit_ctx *)ctx_ptr;

    if (tag->type != MB2_TAG_TYPE_MMAP) {
        return true; /* keep walking */
    }

    ctx->found = true;
    struct mb2_tag_mmap *mmap = (struct mb2_tag_mmap *)tag;
    u32 entry_count = (mmap->size - (u32)sizeof(struct mb2_tag_mmap)) / mmap->entry_size;

    for (u32 i = 0; i < entry_count; i++) {
        struct mb2_mmap_entry *entry =
            (struct mb2_mmap_entry *)((u8 *)mmap->entries + i * mmap->entry_size);

        u64 region_end = entry->addr + entry->len;
        if (region_end > highest_frame_seen) {
            highest_frame_seen = region_end;
        }

        if (entry->type == MB2_MEMORY_AVAILABLE) {
            u64 first_frame = entry->addr / PMM_FRAME_SIZE;
            u64 last_frame = (entry->addr + entry->len) / PMM_FRAME_SIZE;

            for (u64 f = first_frame; f < last_frame && f < MAX_TRACKED_FRAMES; f++) {
                if (bitmap_test(f)) {
                    bitmap_clear(f);
                    free_frames_count++;
                }
            }
        }
    }

    return false; /* found what we need, stop walking */
}

void pmm_init(u64 mb2_info_addr)
{
    /* Start with every tracked frame marked used; the mmap walk
     * below frees exactly the ranges firmware reports as available.
     * This is the safer default direction: an unrecognized or
     * misreported region stays reserved (never handed out) rather
     * than silently becoming allocatable. */
    for (u64 i = 0; i < BITMAP_SIZE_BYTES; i++) {
        frame_bitmap[i] = 0xFF;
    }
    free_frames_count = 0;

    struct mmap_visit_ctx ctx = { .found = false };
    mb2_walk_tags(mb2_info_addr, mmap_tag_visitor, &ctx);

    if (!ctx.found) {
        panic("pmm_init: no Multiboot2 memory map tag present");
    }

    total_frames = highest_frame_seen / PMM_FRAME_SIZE;
    if (total_frames > MAX_TRACKED_FRAMES) {
        total_frames = MAX_TRACKED_FRAMES;
    }

    /* Reserve the kernel's own image and the legacy first 1 MiB
     * (real-mode IVT, BDA, VGA memory, option ROMs) even though a
     * well-behaved firmware's mmap should already mark these
     * reserved -- defense in depth against a mmap that doesn't. */
    reserve_region(0x0, 0x100000);
    reserve_region((paddr_t)(uptr)kernel_start, (paddr_t)(uptr)kernel_end);

    printk_level(LOG_INFO, "pmm: %llu total frames, %llu free (%llu MiB)\n",
                 total_frames, free_frames_count,
                 (free_frames_count * PMM_FRAME_SIZE) / (1024 * 1024));
}

paddr_t pmm_alloc_frame(void)
{
    for (u64 f = 0; f < total_frames; f++) {
        if (!bitmap_test(f)) {
            bitmap_set(f);
            free_frames_count--;
            return f * PMM_FRAME_SIZE;
        }
    }
    return 0; /* out of memory; caller decides how fatal that is */
}

void pmm_free_frame(paddr_t addr)
{
    u64 frame = addr / PMM_FRAME_SIZE;
    if (frame >= total_frames) {
        return; /* address outside tracked range; ignore rather than corrupt */
    }
    if (bitmap_test(frame)) {
        bitmap_clear(frame);
        free_frames_count++;
    }
}

u64 pmm_total_frames(void)
{
    return total_frames;
}

u64 pmm_free_frames(void)
{
    return free_frames_count;
}
