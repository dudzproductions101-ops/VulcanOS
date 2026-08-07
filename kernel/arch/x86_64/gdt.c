/*
 * gdt.c - Global Descriptor Table setup
 *
 * Installs a flat GDT: one code and one data descriptor for ring 0,
 * matching placeholders for ring 3 (unused until user mode exists),
 * and a TSS descriptor. Long mode ignores segment limits/base for
 * code/data (everything is effectively flat), but the descriptors
 * must still be present and correctly typed or the CPU will fault
 * the moment a selector is loaded.
 */

#include "arch/x86_64/gdt.h"

#define GDT_ENTRY_COUNT 7  /* null, kcode, kdata, ucode, udata, tss (2 slots) */

static struct gdt_entry gdt[GDT_ENTRY_COUNT];
static struct tss tss;
static struct gdt_ptr gp;

extern void gdt_flush(u64 gdt_ptr_addr);
extern void tss_flush(void);

static void gdt_set_entry(int idx, u32 base, u32 limit, u8 access, u8 gran)
{
    gdt[idx].base_low    = (u16)(base & 0xFFFF);
    gdt[idx].base_mid    = (u8)((base >> 16) & 0xFF);
    gdt[idx].base_high   = (u8)((base >> 24) & 0xFF);
    gdt[idx].limit_low   = (u16)(limit & 0xFFFF);
    gdt[idx].granularity = (u8)((limit >> 16) & 0x0F);
    gdt[idx].granularity |= (gran & 0xF0);
    gdt[idx].access      = access;
}

static void tss_install(int idx)
{
    u64 base = (u64)&tss;
    u32 limit = sizeof(struct tss) - 1;

    struct tss_entry_low *low = (struct tss_entry_low *)&gdt[idx];
    struct tss_entry_high *high = (struct tss_entry_high *)&gdt[idx + 1];

    low->length   = (u16)(limit & 0xFFFF);
    low->base_low = (u16)(base & 0xFFFF);
    low->base_mid = (u8)((base >> 16) & 0xFF);
    low->flags1   = 0x89; /* present, ring 0, 64-bit TSS (available) */
    low->flags2   = (u8)((limit >> 16) & 0x0F);
    low->base_high = (u8)((base >> 24) & 0xFF);

    high->base_upper = (u32)(base >> 32);
    high->reserved   = 0;

    for (u64 i = 0; i < sizeof(tss); i++) {
        ((u8 *)&tss)[i] = 0;
    }
    tss.iomap_base = sizeof(struct tss);
}

void gdt_install(void)
{
    gp.limit = (sizeof(struct gdt_entry) * GDT_ENTRY_COUNT) - 1;
    gp.base  = (u64)&gdt;

    /* index 0: null descriptor, required by the architecture */
    gdt_set_entry(0, 0, 0, 0, 0);

    /* index 1: kernel code, ring 0, long-mode (access=0x9A, gran=0x20
     * sets the L-bit for 64-bit code; base/limit are ignored by the
     * CPU in long mode but populated for clarity and compatibility) */
    gdt_set_entry(1, 0, 0xFFFFF, 0x9A, 0x20);

    /* index 2: kernel data, ring 0 */
    gdt_set_entry(2, 0, 0xFFFFF, 0x92, 0xC0);

    /* index 3: user code, ring 3 (DPL=3 via access bits 5-6) */
    gdt_set_entry(3, 0, 0xFFFFF, 0xFA, 0x20);

    /* index 4: user data, ring 3 */
    gdt_set_entry(4, 0, 0xFFFFF, 0xF2, 0xC0);

    /* index 5-6: TSS descriptor (occupies two GDT slots in long mode) */
    tss_install(5);

    gdt_flush((u64)&gp);
    tss_flush();
}
