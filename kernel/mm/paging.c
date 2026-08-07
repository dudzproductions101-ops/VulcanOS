/*
 * paging.c - x86_64 4-level page table management
 *
 * See mm/paging.h for why this exists alongside boot.asm's
 * temporary identity map.
 *
 * TERMINOLOGY: x86_64 calls the four levels PML4 -> PDPT -> PD -> PT,
 * mapping progressively smaller regions (512 GiB -> 1 GiB -> 2 MiB ->
 * 4 KiB) at each step. VulcanOS walks/builds this exact hardware
 * layout; the names are Intel's, not a VulcanOS naming choice.
 */

#include "mm/paging.h"
#include "mm/pmm.h"
#include "arch/x86_64/cpu.h"
#include "config.h"
#include "printk.h"
#include "panic.h"

#define ENTRIES_PER_TABLE 512

/* Index into each of the four levels for a given virtual address.
 * x86_64 canonical addressing uses bits 47:39 (PML4), 38:30 (PDPT),
 * 29:21 (PD), 20:12 (PT) -- fixed by the architecture, not a
 * VulcanOS convention. */
#define PML4_INDEX(v) (((v) >> 39) & 0x1FF)
#define PDPT_INDEX(v) (((v) >> 30) & 0x1FF)
#define PD_INDEX(v)   (((v) >> 21) & 0x1FF)
#define PT_INDEX(v)   (((v) >> 12) & 0x1FF)

typedef u64 page_table_t[ENTRIES_PER_TABLE];

static page_table_t *kernel_pml4 = NULL;

/* Physical addresses are identity-accessible right up until this
 * module remaps things, and remain so afterward for the low region
 * we deliberately keep identity-mapped -- see paging_init. This
 * lets us treat "physical address" and "pointer we can dereference"
 * as the same thing throughout early mm bring-up, which is only
 * safe because of that deliberate identity range, not in general. */
static inline page_table_t *phys_to_table_ptr(paddr_t phys)
{
    return (page_table_t *)(uptr)phys;
}

/* Walks from `table` down to the PT level for `virt`, allocating any
 * missing intermediate table via pmm_alloc_frame as it goes. Returns
 * a pointer to the PT-level entry for `virt`, or NULL if allocation
 * failed partway through (physical memory exhausted). */
static u64 *walk_and_create(page_table_t *pml4, vaddr_t virt)
{
    page_table_t *table = pml4;

    for (int level = 0; level < 3; level++) {
        u64 index;
        switch (level) {
        case 0: index = PML4_INDEX(virt); break;
        case 1: index = PDPT_INDEX(virt); break;
        default: index = PD_INDEX(virt); break;
        }

        u64 entry = (*table)[index];

        if (!(entry & PAGE_PRESENT)) {
            paddr_t new_table_phys = pmm_alloc_frame();
            if (new_table_phys == 0) {
                return NULL; /* out of physical memory */
            }

            page_table_t *new_table = phys_to_table_ptr(new_table_phys);
            for (int i = 0; i < ENTRIES_PER_TABLE; i++) {
                (*new_table)[i] = 0;
            }

            (*table)[index] = new_table_phys | PAGE_PRESENT | PAGE_WRITABLE;
            table = new_table;
        } else {
            table = phys_to_table_ptr(entry & PAGE_ADDR_MASK);
        }
    }

    return &(*table)[PT_INDEX(virt)];
}

bool paging_map_page(vaddr_t virt, paddr_t phys, u64 flags)
{
    u64 *pte = walk_and_create(kernel_pml4, virt);
    if (!pte) {
        return false;
    }

    *pte = (phys & PAGE_ADDR_MASK) | flags | PAGE_PRESENT;

    /* Invalidate this page's stale TLB entry, if any existed from a
     * previous mapping at this virtual address. A fresh mapping
     * that was never cached doesn't strictly need this, but paying
     * one invlpg unconditionally is far cheaper than tracking
     * "was this address ever mapped before" just to skip it. */
    __asm__ volatile ("invlpg (%0)" : : "r"(virt) : "memory");

    return true;
}

void paging_unmap_page(vaddr_t virt)
{
    u64 *pte = walk_and_create(kernel_pml4, virt);
    if (pte) {
        *pte = 0;
        __asm__ volatile ("invlpg (%0)" : : "r"(virt) : "memory");
    }
}

paddr_t paging_virt_to_phys(vaddr_t virt)
{
    page_table_t *table = kernel_pml4;

    for (int level = 0; level < 3; level++) {
        u64 index;
        switch (level) {
        case 0: index = PML4_INDEX(virt); break;
        case 1: index = PDPT_INDEX(virt); break;
        default: index = PD_INDEX(virt); break;
        }

        u64 entry = (*table)[index];
        if (!(entry & PAGE_PRESENT)) {
            return 0;
        }
        table = phys_to_table_ptr(entry & PAGE_ADDR_MASK);
    }

    u64 pte = (*table)[PT_INDEX(virt)];
    if (!(pte & PAGE_PRESENT)) {
        return 0;
    }

    return (pte & PAGE_ADDR_MASK) | (virt & 0xFFF);
}

static void enable_nx_bit(void)
{
    /* EFER.NXE (bit 11) must be set before any PTE's NX bit (63) is
     * honored by the CPU; without this, setting NX on a page just
     * silently reserves the bit rather than enforcing it. */
    u32 eax, edx;
    __asm__ volatile ("rdmsr" : "=a"(eax), "=d"(edx) : "c"(0xC0000080));
    eax |= (1 << 11);
    __asm__ volatile ("wrmsr" : : "a"(eax), "d"(edx), "c"(0xC0000080));
}

void paging_init(paddr_t kernel_phys_start, paddr_t kernel_phys_end)
{
    enable_nx_bit();

    paddr_t pml4_phys = pmm_alloc_frame();
    if (pml4_phys == 0) {
        panic("paging_init: could not allocate PML4");
    }

    kernel_pml4 = phys_to_table_ptr(pml4_phys);
    for (int i = 0; i < ENTRIES_PER_TABLE; i++) {
        (*kernel_pml4)[i] = 0;
    }

    /* Identity-map the first 16 MiB: legacy hardware (VGA memory,
     * PIC/PIT/keyboard controller access is port I/O and unaffected,
     * but early boot structures and the kernel's own low-memory
     * bootstrap data live here) and, critically, the physical pages
     * this very function is about to allocate for page tables --
     * they must stay dereferenceable through their physical address
     * even after CR3 is swapped to this new PML4. Chosen generously
     * (16 MiB, not just 1 MiB) to leave headroom for early PMM
     * bitmap/table allocations that land just past the kernel image
     * before a proper physical-memory allocator strategy exists. */
    for (paddr_t addr = 0; addr < 16 * 1024 * 1024; addr += PMM_FRAME_SIZE) {
        if (!paging_map_page((vaddr_t)addr, addr, PAGE_PRESENT | PAGE_WRITABLE)) {
            panic("paging_init: failed to build identity map");
        }
    }

    /* Map the kernel image into the higher half. .text/.rodata get
     * read-only(ish) treatment is a future refinement once per-
     * section boundaries are threaded through from the linker
     * script; for this milestone the whole image is mapped
     * writable+executable at its higher-half address, which is
     * still strictly better than boot.asm's blanket 2 MiB RWX
     * identity pages since at least it separates kernel-space from
     * a future user-space's low addresses entirely. */
    u64 image_size = kernel_phys_end - kernel_phys_start;
    for (u64 offset = 0; offset < image_size; offset += PMM_FRAME_SIZE) {
        vaddr_t virt = VULCAN_KERNEL_VBASE + offset;
        paddr_t phys = kernel_phys_start + offset;
        if (!paging_map_page(virt, phys, PAGE_PRESENT | PAGE_WRITABLE)) {
            panic("paging_init: failed to map kernel higher-half image");
        }
    }

    write_cr3(pml4_phys);

    printk_level(LOG_INFO, "paging: NX enabled, kernel mapped at 0x%llx (%llu KiB), "
                 "identity map through 16 MiB\n",
                 VULCAN_KERNEL_VBASE, image_size / 1024);
}
