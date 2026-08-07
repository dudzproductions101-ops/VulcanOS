











#include "mm/allocator.h"
#include "mm/paging.h"
#include "mm/pmm.h"
#include "printk.h"
#include "panic.h"




#define KHEAP_VBASE (0xFFFFFFFF90000000ULL)
#define KHEAP_MAX_SIZE (256ULL * 1024 * 1024) 

#define BLOCK_MAGIC 0x4B4845415041u 

struct block_header {
    u64 magic;
    usize size;              
    bool free;
    struct block_header *next_free;
};

static vaddr_t heap_current_end = 0;   
static struct block_header *free_list = NULL;

static bool heap_grow(usize min_size)
{
    usize needed = min_size + sizeof(struct block_header);
    usize pages_needed = (needed + PMM_FRAME_SIZE - 1) / PMM_FRAME_SIZE;

    if (heap_current_end + pages_needed * PMM_FRAME_SIZE - KHEAP_VBASE > KHEAP_MAX_SIZE) {
        return false; 
    }

    vaddr_t start = heap_current_end;
    for (usize i = 0; i < pages_needed; i++) {
        paddr_t frame = pmm_alloc_frame();
        if (frame == 0) {
            return false; 
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

    




}
