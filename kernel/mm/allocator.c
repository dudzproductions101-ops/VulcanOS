/*
 * allocator.c - Kernel heap allocator (kmalloc/kfree)
 *
 * See mm/allocator.h for the first-fit-over-slab design rationale.
 *
 * Layout: a singly-linked list of blocks, each preceded by a
 * block_header. Free blocks are linked via `next_free`; allocated
 * blocks are not in that list at all (rather than a "free" flag
 * that free-list code has to remember to check), so walking the
 * free list can never accidentally hand out a block still in use.
 */

#include "mm/allocator.h"
#include "mm/paging.h"
#include "mm/pmm.h"
#include "printk.h"
#include "panic.h"

/* Placed well above the kernel image itself (which starts at
 * VULCAN_KERNEL_VBASE) so heap growth can never collide with
 * kernel code/data as the image grows across future development. */
#define KHEAP_VBASE (0xFFFFFFFF90000000ULL)
#define KHEAP_MAX_SIZE (256ULL * 1024 * 1024) /* 256 MiB address-space ceiling */

#define BLOCK_MAGIC 0x4B4845415041u /* "KHEAPA" truncated to fit; sanity-check only */

struct block_header {
    u64 magic;
    usize size;              /* usable size, not including this header */
    bool free;
    struct block_header *next_free;
};

static vaddr_t heap_current_end = 0;   /* first not-yet-mapped virtual address */
static struct block_header *free_list = NULL;

static bool heap_grow(usize min_size)
{
    usize needed = min_size + sizeof(struct block_header);
    usize pages_needed = (needed + PMM_FRAME_SIZE - 1) / PMM_FRAME_SIZE;

    if (heap_current_end + pages_needed * PMM_FRAME_SIZE - KHEAP_VBASE > KHEAP_MAX_SIZE) {
        return false; /* would exceed the heap's reserved address range */
    }

    vaddr_t start = heap_current_end;
    for (usize i = 0; i < pages_needed; i++) {
        paddr_t frame = pmm_alloc_frame();
        if (frame == 0) {
            return false; /* physical memory exhausted partway through growth */
        }
        if (!paging_map_page(heap_current_end, frame, PAGE_PRESENT | PAGE_WRITABLE)) {
            pmm_free_frame(frame);
            return false;
        }
        heap_current_end += PMM_FRAME_SIZE;
    }

    struct block_header *new_block = (struct block_header *)start;
    new_block->magic = BLOCK_MAGIC;
    new_block->size = pages_needed * PMM_FRAME_SIZE - sizeof(struct block_header);
    new_block->free = true;
    new_block->next_free = free_list;
    free_list = new_block;

    return true;
}

void kheap_init(void)
{
    heap_current_end = KHEAP_VBASE;
    free_list = NULL;
    printk_level(LOG_INFO, "kheap: reserved at 0x%llx, growable to %llu MiB\n",
                 KHEAP_VBASE, KHEAP_MAX_SIZE / (1024 * 1024));
}

/* Simple 16-byte alignment for all returned pointers -- sufficient
 * for every scalar type on x86_64 and for SSE-aligned data should
 * VulcanOS's freestanding code ever re-enable SSE (currently
 * disabled kernel-wide via -mno-sse, see kernel/Makefile). */
static inline usize align_up(usize n, usize align)
{
    return (n + align - 1) & ~(align - 1);
}

void *kmalloc(usize size)
{
    if (size == 0) {
        return NULL;
    }
    size = align_up(size, 16);

    struct block_header *prev = NULL;
    struct block_header *curr = free_list;

    while (curr) {
        if (curr->free && curr->size >= size) {
            /* Split the block if there's enough room left over to
             * form another usable free block (i.e. more than just
             * a header's worth of slack), so we don't waste large
             * allocations by handing out an oversized block whole. */
            if (curr->size >= size + sizeof(struct block_header) + 16) {
                struct block_header *remainder =
                    (struct block_header *)((u8 *)curr + sizeof(struct block_header) + size);
                remainder->magic = BLOCK_MAGIC;
                remainder->size = curr->size - size - sizeof(struct block_header);
                remainder->free = true;
                remainder->next_free = curr->next_free;

                curr->size = size;
                curr->next_free = remainder;
            }

            curr->free = false;

            /* Unlink curr from the free list. */
            if (prev) {
                prev->next_free = curr->next_free;
            } else {
                free_list = curr->next_free;
            }
            curr->next_free = NULL;

            return (void *)((u8 *)curr + sizeof(struct block_header));
        }

        prev = curr;
        curr = curr->next_free;
    }

    /* Nothing big enough in the free list; grow the heap and retry
     * exactly once. If growth itself fails, we are genuinely out of
     * either address space or physical memory. */
    if (!heap_grow(size)) {
        return NULL;
    }
    return kmalloc(size);
}

void kfree(void *ptr)
{
    if (!ptr) {
        return;
    }

    struct block_header *block =
        (struct block_header *)((u8 *)ptr - sizeof(struct block_header));

    if (block->magic != BLOCK_MAGIC) {
        panic("kfree: corrupted or invalid heap pointer");
    }
    if (block->free) {
        panic("kfree: double free detected");
    }

    block->free = true;
    block->next_free = free_list;
    free_list = block;

    /* NOTE: does not coalesce adjacent free blocks. Under sustained
     * alloc/free churn this will fragment; coalescing (or a more
     * structured allocator) is a documented follow-up once real
     * allocation patterns exist to design it around, rather than
     * guessed at now. */
}
