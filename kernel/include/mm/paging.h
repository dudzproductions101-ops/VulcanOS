














#ifndef VULCAN_MM_PAGING_H
#define VULCAN_MM_PAGING_H

#include "types.h"


#define PAGE_PRESENT   (1ULL << 0)
#define PAGE_WRITABLE  (1ULL << 1)
#define PAGE_USER      (1ULL << 2)
#define PAGE_HUGE      (1ULL << 7)   
#define PAGE_NO_EXEC   (1ULL << 63)  

#define PAGE_ADDR_MASK 0x000FFFFFFFFFF000ULL







void paging_init(paddr_t kernel_phys_start, paddr_t kernel_phys_end);






bool paging_map_page(vaddr_t virt, paddr_t phys, u64 flags);





void paging_unmap_page(vaddr_t virt);





paddr_t paging_virt_to_phys(vaddr_t virt);

#endif 
