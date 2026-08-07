/*
 * printk.h - Kernel-space formatted output
 *
 * A from-scratch, minimal printf-family implementation for the
 * kernel. Deliberately NOT the same code as libc/printf.c: the
 * kernel cannot link against user-space libc (different linking
 * context, no heap available at early boot, and the kernel must
 * remain self-contained so it never depends on the C library it
 * provides to user space). Some duplication between printk's
 * formatter and libc's is an accepted tradeoff of freestanding
 * kernel development, not an oversight.
 */

#ifndef VULCAN_PRINTK_H
#define VULCAN_PRINTK_H

#include "types.h"

/* Log levels control both a "[LEVEL]" prefix and, on ports that grow
 * a ring buffer/log filtering later, which messages are recorded. */
enum log_level {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
};

void printk(const char *fmt, ...);
void printk_level(enum log_level level, const char *fmt, ...);

#endif /* VULCAN_PRINTK_H */
