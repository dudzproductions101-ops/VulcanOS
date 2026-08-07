#include "mm/paging.h"
#include "mm/pmm.h"
#include "arch/x86_64/cpu.h"
#include "config.h"
#include "printk.h"
#include "panic.h"

#define ENTRIES_PER_TABLE 512

#define PML4_INDEX(v) (((v) >> 39) & 0x1FF)
#define PDPT_INDEX(v) (((v) >> 30) & 0x1FF)
#define PD_INDEX(v)   (((v) >> 21) & 0x1FF)
#define PT_INDEX(v)   (((v) >> 12) & 0x1FF)

typedef u64 page_table_t[ENTRIES_PER_TABLE];

static page_table_t *kernel_pml4 = NULL;

static inline page_table_t *phys_to_table_ptr(paddr_t phys)
{
    return (page_table_t *)(uptr)phys;
}

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
                return NULL; 
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

    for (paddr_t addr = 0; addr < 16 * 1024 * 1024; addr += PMM_FRAME_SIZE) {
        if (!paging_map_page((vaddr_t)addr, addr, PAGE_PRESENT | PAGE_WRITABLE)) {
            panic("paging_init: failed to build identity map");
        }
    }

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
