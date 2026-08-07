/*
 * stdlib.h - VulcanOS libc general utilities
 *
 * malloc/free/exit here call directly into kernel functions
 * (kmalloc/kfree/process_exit) rather than crossing a real syscall
 * boundary -- see stdio.h's file comment for the full explanation
 * of why, and what changes once VulcanOS has ring-3 execution.
 */

#ifndef VULCAN_LIBC_STDLIB_H
#define VULCAN_LIBC_STDLIB_H

#include "vulcan_types.h"

void *malloc(usize size);
void free(void *ptr);

int atoi(const char *s);
long atol(const char *s);

/* Terminates the calling thread with the given status. Named exit()
 * to match the standard C name userland code expects, even though
 * what actually happens underneath (marking the current thread's
 * owning process exited and yielding to the scheduler -- see
 * stdlib.c) is VulcanOS-specific. Never returns. */
__attribute__((noreturn)) void exit(int status);

/* Voluntarily gives up the remainder of the calling thread's
 * current time slice, letting the scheduler run something else
 * immediately rather than waiting for the next timer-tick
 * preemption. Wraps proc/scheduler.h's scheduler_yield() -- exposed
 * through libc rather than requiring userland code to reach into
 * kernel headers directly, keeping the same clean layering every
 * other libc function in this file maintains. The primary consumer
 * right now is vulsh's input loop (user/shell/vulsh.c), which polls
 * a non-blocking stdin (see read()'s design note above read()'s own
 * implementation in stdio.c) and needs to yield between polls
 * rather than busy-spin and starve other threads of their fair
 * rotation. */
void yield(void);

/* Blocks the calling thread for approximately `seconds` seconds.
 * The current implementation is scheduler-aware and uses the PIT
 * tick counter; it is a real blocking sleep, not a busy-wait.
 */
void sleep(int seconds);

#endif /* VULCAN_LIBC_STDLIB_H */
