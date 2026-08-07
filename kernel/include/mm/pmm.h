













#ifndef VULCAN_MM_PMM_H
#define VULCAN_MM_PMM_H

#include "types.h"

#define PMM_FRAME_SIZE 4096







void pmm_init(u64 mb2_info_addr);






paddr_t pmm_alloc_frame(void);




void pmm_free_frame(paddr_t addr);

u64 pmm_total_frames(void);
u64 pmm_free_frames(void);

#endif 
