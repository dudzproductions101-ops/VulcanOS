/*
 * allocator.h - Kernel heap allocator (kmalloc/kfree)
 *
 * A first-fit free-list allocator over a fixed virtual address
 * range, grown one page at a time via paging_map_page as needed.
 * Chosen over a slab allocator for this milestone because VulcanOS
 * doesn't yet have enough distinct, high-frequency fixed-size
 * allocation patterns (that's what slabs are for) to justify one --
 * the scheduler's task_struct pool is the first candidate once it
 * exists, and a slab allocator specifically for that is a
 * reasonable follow-up rather than generalizing kmalloc prematurely.
 */

#ifndef VULCAN_MM_ALLOCATOR_H
#define VULCAN_MM_ALLOCATOR_H

#include "types.h"

void kheap_init(void);
void *kmalloc(usize size);
void kfree(void *ptr);

#endif /* VULCAN_MM_ALLOCATOR_H */
