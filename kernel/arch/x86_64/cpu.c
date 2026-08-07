/*
 * cpu.c - CPU identification
 *
 * Most CPU primitives (port I/O, hlt, cli/sti) are simple enough to
 * live as `static inline` functions directly in cpu.h. This file
 * holds the ones that don't belong in a header: CPUID-based vendor/
 * feature detection, used both by early boot sanity checks (does
 * this CPU even support long mode?) and later by vulcaninfo to
 * report real hardware info instead of guessing.
 */

#include "arch/x86_64/cpu.h"
#include "printk.h"

static void cpuid(u32 leaf, u32 *eax, u32 *ebx, u32 *ecx, u32 *edx)
{
    __asm__ volatile ("cpuid"
                       : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                       : "a"(leaf));
}

void cpu_get_vendor(struct cpu_vendor *out)
{
    u32 eax, ebx, ecx, edx;
    cpuid(0, &eax, &ebx, &ecx, &edx);

    /* CPUID leaf 0 returns the vendor string spread across
     * ebx/edx/ecx, in that specific order (an Intel-defined
     * quirk, not alphabetical or size order). */
    *(u32 *)&out->id[0] = ebx;
    *(u32 *)&out->id[4] = edx;
    *(u32 *)&out->id[8] = ecx;
    out->id[12] = '\0';
}

/* Returns true if the CPU advertises long-mode (64-bit) support via
 * the extended CPUID leaf. VulcanOS's boot.asm checks this before
 * attempting the transition into long mode; if boot.asm reaches
 * kmain at all, this will always report true, but the check remains
 * useful for diagnostics and for vulcaninfo. */
bool cpu_supports_long_mode(void)
{
    u32 eax, ebx, ecx, edx;

    /* First confirm extended leaves are available at all. */
    cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
    if (eax < 0x80000001) {
        return false;
    }

    cpuid(0x80000001, &eax, &ebx, &ecx, &edx);
    return (edx & (1 << 29)) != 0; /* LM bit */
}

void cpu_print_info(void)
{
    struct cpu_vendor vendor;
    cpu_get_vendor(&vendor);
    printk("cpu: vendor=%s long_mode=%s\n",
           vendor.id,
           cpu_supports_long_mode() ? "yes" : "no");
}
