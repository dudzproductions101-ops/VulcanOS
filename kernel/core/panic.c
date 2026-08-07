/*
 * panic.c - Unrecoverable kernel error handling
 *
 * NOTE: the version of this file originally in the archive included
 * "kernel.c" (a .c file including another .c file, which does not
 * compile as a translation unit) and kernel.c included this file
 * right back, forming a circular include. This version includes
 * only headers, matching the actual dependency: panic() needs
 * printk() and needs to halt the CPU, nothing more.
 */

#include "panic.h"
#include "printk.h"
#include "arch/x86_64/cpu.h"

void panic(const char *msg)
{
    printk_level(LOG_ERROR, "KERNEL PANIC: %s\n", msg);
    printk("system halted.\n");
    halt_forever();
}
