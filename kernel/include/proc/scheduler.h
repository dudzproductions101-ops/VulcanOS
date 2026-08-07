/*
 * scheduler.h - Round-robin thread scheduler
 *
 * A fixed-time-slice round-robin scheduler: every ready thread gets
 * an equal turn, in a fixed rotation, no priority levels. This is
 * the correct first scheduler for a bring-up milestone -- simple
 * enough to verify by reading the ready-queue rotation logic
 * directly, with no starvation possible by construction (every
 * thread is guaranteed to reach the front of the queue eventually).
 * Priority levels, multi-level feedback, or a CFS-style fair
 * scheduler are reasonable upgrades once there's a real multi-
 * process workload to design them against -- not before, since
 * tuning fairness heuristics against an imagined workload tends to
 * produce a scheduler tuned for nothing real.
 */

#ifndef VULCAN_PROC_SCHEDULER_H
#define VULCAN_PROC_SCHEDULER_H

#include "types.h"
#include "proc/thread.h"

/* Time slice length in timer ticks. At the 100 Hz PIT rate
 * (PIT_FREQUENCY_HZ, drivers/timer.h), 5 ticks = 50ms per thread's
 * turn -- long enough that context-switch overhead is negligible
 * relative to useful work, short enough that the system still feels
 * responsive to a human at the console. */
#define SCHEDULER_TIME_SLICE_TICKS 5

void scheduler_init(void);

/* Adds a ready thread to the round-robin rotation. See thread.h's
 * thread_create comment for why this is a separate step from
 * construction. */
void scheduler_enqueue(struct thread *t);

/* Called from the timer IRQ handler on every tick (timer.c). Tracks
 * the current thread's remaining time slice and, if it has expired,
 * triggers a context switch to the next ready thread. Safe to call
 * every tick even when no switch is due -- it is simply a no-op
 * decrement in that case. */
void scheduler_tick(void);

/* Voluntarily yields the CPU before the current time slice expires
 * (e.g. a thread that's about to block on I/O). Immediately
 * triggers a context switch rather than waiting for the next timer
 * tick. */
void scheduler_yield(void);

/* Blocks the current thread until the PIT reaches `wake_tick`. The
 * caller must have already computed the absolute wake tick value via
 * timer_ticks() + delay. This is the primitive that powers real
 * blocking sleep in a scheduler that already supports BLOCKED state.
 */
void scheduler_sleep(u64 wake_tick);

/* Returns the thread currently executing on this CPU, or NULL if
 * the scheduler has not started yet (see scheduler_start). */
struct thread *scheduler_current(void);

/* Marks the calling thread as no longer ready to run (state ==
 * THREAD_BLOCKED or THREAD_DEAD must already be set by the caller)
 * and switches to the next ready thread. Never returns if the
 * thread is DEAD; returns normally, resuming the caller, if some
 * other code later re-enqueues it after BLOCKED. */
void scheduler_reschedule(void);

/* Hands control to the scheduler for the first time. Never returns
 * under normal operation -- from this point on, every future return
 * to "the idle loop" happens because the scheduler picked the idle
 * thread's turn, not because this function's call stack unwound. */
__attribute__((noreturn)) void scheduler_start(void);

#endif /* VULCAN_PROC_SCHEDULER_H */
