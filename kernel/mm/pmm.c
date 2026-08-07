















#include "mm/pmm.h"
#include "multiboot2.h"
#include "printk.h"
#include "panic.h"





#define MAX_TRACKED_FRAMES (4ULL * 1024 * 1024 * 1024 / PMM_FRAME_SIZE)
#define BITMAP_SIZE_BYTES (MAX_TRACKED_FRAMES / 8)

static u8 frame_bitmap[BITMAP_SIZE_BYTES];
static u64 total_frames = 0;
static u64 free_frames_count = 0;
static u64 highest_frame_seen = 0;




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
        return true; 
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

    return false; 
}

void pmm_init(u64 mb2_info_addr)
{
    




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
    return 0; 
}

void pmm_free_frame(paddr_t addr)
{
    u64 frame = addr / PMM_FRAME_SIZE;
    if (frame >= total_frames) {
        return; 
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
