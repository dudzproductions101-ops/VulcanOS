#ifndef VULCAN_MM_ALLOCATOR_H
#define VULCAN_MM_ALLOCATOR_H

#include "types.h"

void kheap_init(void);
void *kmalloc(usize size);
void kfree(void *ptr);

#endif 
