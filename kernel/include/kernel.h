/*
 * kernel.h - Top-level kernel declarations
 *
 * The kernel's main entry point and any genuinely global helpers
 * live here. This header intentionally stays small: subsystem
 * headers (printk.h, panic.h, gdt.h, ...) are included directly by
 * the .c files that need them rather than re-exported through here,
 * so #include graphs stay traceable instead of everything silently
 * depending on everything through one god-header.
 */

#ifndef VULCAN_KERNEL_H
#define VULCAN_KERNEL_H

#include "types.h"

/* Entry point called by boot.asm once long mode is active and a
 * valid stack is set up. Never returns under normal operation: on
 * a bring-up kernel with no scheduler yet, falls through to the
 * idle loop once initialization completes.
 *
 * mb2_magic/mb2_info_addr are exactly what GRUB left in eax/ebx per
 * the Multiboot2 spec, passed through unmodified by long_mode_start
 * in boot.asm via the System V AMD64 rdi/rsi argument registers. */
void kmain(u64 mb2_magic, u64 mb2_info_addr);

#endif /* VULCAN_KERNEL_H */
