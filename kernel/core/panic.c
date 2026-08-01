
#include "panic.h"
#include "printk.h"
#include "arch/x86_64/cpu.h"

void panic(const char *msg)
{
    printk_level(LOG_ERROR, "KERNEL PANIC: %s\n", msg);
    printk("system halted.\n");
    halt_forever();
}
