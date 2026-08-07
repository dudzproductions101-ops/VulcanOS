/*
 * paging.h - x86_64 4-level page table management
 *
 * boot.asm builds a temporary identity map (first 1 GiB, 2 MiB
 * pages) just to get from real/protected mode into long mode with
 * paging active at all -- it is deliberately minimal scaffolding,
 * not VulcanOS's real memory layout. This module builds VulcanOS's
 * actual virtual address space: a higher-half kernel (mapped at
 * VULCAN_KERNEL_VBASE, see config.h) with 4 KiB page granularity,
 * so per-page permissions (read-only .rodata, no-execute .data/.bss,
 * eventually per-process user mappings) are enforceable rather than
 * the coarse "everything is RWX in one 2 MiB block" the boot-time
 * map provides.
 */

#ifndef VULCAN_MM_PAGING_H
#define VULCAN_MM_PAGING_H

#include "types.h"

/* Standard x86_64 page table entry flags (Intel SDM Vol. 3, 4.5). */
#define PAGE_PRESENT   (1ULL << 0)
#define PAGE_WRITABLE  (1ULL << 1)
#define PAGE_USER      (1ULL << 2)
#define PAGE_HUGE      (1ULL << 7)   /* 2 MiB/1 GiB page at PD/PDPT level */
#define PAGE_NO_EXEC   (1ULL << 63)  /* requires EFER.NXE, set in paging_init */

#define PAGE_ADDR_MASK 0x000FFFFFFFFFF000ULL

/* Initializes VulcanOS's real page tables: enables the NX bit,
 * builds a fresh PML4, identity-maps the first few MiB (so the
 * kernel doesn't lose access to itself the instant it switches
 * CR3), maps the kernel image at VULCAN_KERNEL_VBASE with correct
 * per-section permissions, and loads the new PML4 into CR3.
 * kernel_phys_start/kernel_phys_end come from linker.ld. */
void paging_init(paddr_t kernel_phys_start, paddr_t kernel_phys_end);

/* Maps a single 4 KiB page, allocating any intermediate page-table
 * levels (PDPT/PD/PT) on demand via pmm_alloc_frame. Returns false
 * if an intermediate table couldn't be allocated (physical memory
 * exhausted) -- callers must check this rather than assume mapping
 * always succeeds. */
bool paging_map_page(vaddr_t virt, paddr_t phys, u64 flags);

/* Removes a single page's mapping. Does not free the underlying
 * physical frame -- that is the caller's responsibility via
 * pmm_free_frame, since paging.c has no way to know whether the
 * frame is still referenced elsewhere. */
void paging_unmap_page(vaddr_t virt);

/* Returns the physical address a virtual address currently maps to,
 * or 0 if unmapped. Used by page-fault diagnostics and by any code
 * that needs to translate a kernel pointer back to a physical
 * address (e.g. for a future DMA-capable driver). */
paddr_t paging_virt_to_phys(vaddr_t virt);

#endif /* VULCAN_MM_PAGING_H */
