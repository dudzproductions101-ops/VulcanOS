
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

    *(u32 *)&out->id[0] = ebx;
    *(u32 *)&out->id[4] = edx;
    *(u32 *)&out->id[8] = ecx;
    out->id[12] = '\0';
}

bool cpu_supports_long_mode(void)
{
    u32 eax, ebx, ecx, edx;

    cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
    if (eax < 0x80000001) {
        return false;
    }

    cpuid(0x80000001, &eax, &ebx, &ecx, &edx);
    return (edx & (1 << 29)) != 0;
}

void cpu_print_info(void)
{
    struct cpu_vendor vendor;
    cpu_get_vendor(&vendor);
    printk("cpu: vendor=%s long_mode=%s\n",
           vendor.id,
           cpu_supports_long_mode() ? "yes" : "no");
}
