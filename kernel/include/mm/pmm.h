/*
 * pmm.h - Physical Memory Manager
 *
 * Tracks which 4 KiB physical frames are free using a bitmap: one
 * bit per frame, 0 = free, 1 = used. A bitmap is the right first
 * implementation for VulcanOS's mm bring-up milestone -- it is
 * trivial to verify by inspection (unlike a buddy allocator's
 * order-splitting logic) and Multiboot2 gives us the full memory
 * map up front, so there is no benefit yet to a scheme optimized
 * for concurrent/incremental allocation. A buddy or slab allocator
 * built on top of this is a reasonable Phase 3+ upgrade once real
 * workloads exist to profile against.
 */

#ifndef VULCAN_MM_PMM_H
#define VULCAN_MM_PMM_H

#include "types.h"

#define PMM_FRAME_SIZE 4096

/* Initializes the frame bitmap from the Multiboot2 memory map,
 * marks all frames used by default, then frees exactly the frames
 * the firmware reported as MB2_MEMORY_AVAILABLE. Also reserves the
 * kernel's own physical footprint (kernel_start..kernel_end, see
 * linker.ld) and the first 1 MiB, so those frames are never handed
 * out even if the firmware's map is silent about them. */
void pmm_init(u64 mb2_info_addr);

/* Allocates one free 4 KiB frame and returns its physical address,
 * or 0 if physical memory is exhausted. Callers must check for 0;
 * pmm_alloc_frame() does not panic on OOM, because whether OOM is
 * fatal is a policy decision for the caller (a kernel-critical
 * allocation vs. a user process's malloc are not equally fatal). */
paddr_t pmm_alloc_frame(void);

/* Marks a previously-allocated frame free again. Undefined behavior
 * (in the "will corrupt the bitmap" sense, not memory-unsafe) if
 * passed an address that was never returned by pmm_alloc_frame. */
void pmm_free_frame(paddr_t addr);

u64 pmm_total_frames(void);
u64 pmm_free_frames(void);

#endif /* VULCAN_MM_PMM_H */
