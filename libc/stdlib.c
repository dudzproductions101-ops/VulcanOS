/*
 * stdlib.c - VulcanOS libc general utilities
 *
 * INTERIM DESIGN NOTE (applies to this entire libc, documented once
 * here in full since every I/O-touching file references it):
 * VulcanOS has no ring-3 (user-mode) execution yet -- every process
 * created so far (idle, demo-a, demo-b; see kernel/core/kernel.c)
 * runs at ring 0, sharing the kernel's own address space directly
 * (see the file comment in kernel/include/proc/process.h). There is
 * no SYSCALL/SYSRET or interrupt-gate syscall mechanism, no
 * privilege-level transition, and no memory isolation between
 * "kernel code" and "user code" -- there is only kernel code right
 * now, some of which happens to be organized as if it were a
 * separate userland.
 *
 * Given that, this libc's I/O and memory functions (malloc, free,
 * exit here; open/read/write/close in stdio.c) call DIRECTLY into
 * the kernel functions they'd otherwise reach via a syscall --
 * kmalloc, kfree, vfs_open, and so on. This is not a syscall
 * emulation layer or a shim; it is an honest reflection of the
 * current architecture: there is no boundary to cross yet, so
 * nothing here pretends to cross one.
 *
 * This is a real, explicit interim decision, not an oversight --
 * matching the same pattern already documented for GRUB-instead-of-
 * a-native-bootloader and vulcanfs-as-RAM-resident (see
 * PROJECT_STATUS.md). Building a genuine ring-3 syscall mechanism
 * (a real user/kernel privilege split, SYSCALL/SYSRET or an
 * interrupt-gate ABI, per-process page tables enforcing isolation)
 * is real, substantial, security-relevant future work -- deliberately
 * scoped as its own phase rather than rushed to unblock this one.
 * When that phase lands, every function in this file changes from
 * "calls a kernel function directly" to "traps into the kernel via
 * the real syscall instruction" -- the function SIGNATURES here
 * (malloc(size), exit(status), ...) are written to already match
 * what userland code should look like on the other side of that
 * transition, so application code written against this libc today
 * should not need to change when that lands.
 */

#include "stdlib.h"
#include "drivers/timer.h"
#include "mm/allocator.h"
#include "proc/scheduler.h"
#include "proc/process.h"
#include "proc/thread.h"

void *malloc(usize size)
{
    return kmalloc(size);
}

void free(void *ptr)
{
    kfree(ptr);
}

int atoi(const char *s)
{
    return (int)atol(s);
}

long atol(const char *s)
{
    long result = 0;
    bool negative = false;

    while (*s == ' ' || *s == '\t' || *s == '\n') {
        s++;
    }

    if (*s == '-') {
        negative = true;
        s++;
    } else if (*s == '+') {
        s++;
    }

    while (*s >= '0' && *s <= '9') {
        result = result * 10 + (*s - '0');
        s++;
    }

    return negative ? -result : result;
}

__attribute__((noreturn)) void exit(int status)
{
    struct thread *self = scheduler_current();

    if (self && self->owner) {
        process_exit(self->owner, status);
    }

    self->state = THREAD_DEAD;
    scheduler_reschedule();

    /* scheduler_reschedule never returns for a DEAD thread (see its
     * contract in scheduler.h) -- panic rather than fall through to
     * undefined behavior if that assumption is ever violated. */
    extern __attribute__((noreturn)) void panic(const char *msg);
    panic("exit: scheduler_reschedule returned for a DEAD thread");
}

void yield(void)
{
    scheduler_yield();
}

void sleep(int seconds)
{
    if (seconds <= 0) {
        return;
    }

    u64 wake_tick = timer_ticks() + (u64)seconds * PIT_FREQUENCY_HZ;
    scheduler_sleep(wake_tick);
}
