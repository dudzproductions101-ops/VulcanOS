/*
 * scheduler.c - Round-robin thread scheduler
 *
 * See proc/scheduler.h for the round-robin-over-priority design
 * rationale. Implementation note: the ready queue is a circular
 * singly-linked list using struct thread's own `next` field (no
 * separate queue node allocation needed) with `ready_queue` always
 * pointing at the thread whose turn is current or next.
 */

#include "proc/scheduler.h"
#include "proc/thread.h"
#include "arch/x86_64/cpu.h"
#include "drivers/timer.h"
#include "mm/allocator.h"
#include "printk.h"
#include "panic.h"

struct sleeping_thread {
    struct thread *thread;
    u64 wake_tick;
    struct sleeping_thread *next;
};

static struct thread *ready_queue = NULL;   /* circular list; NULL if empty */
static struct thread *current = NULL;
static u32 ticks_remaining = 0;
static bool scheduler_running = false;
static struct sleeping_thread *sleeping_threads = NULL;

void scheduler_init(void)
{
    ready_queue = NULL;
    current = NULL;
    ticks_remaining = 0;
    scheduler_running = false;
    printk_level(LOG_INFO, "scheduler: initialized (round-robin, %d-tick slices)\n",
                 SCHEDULER_TIME_SLICE_TICKS);
}

void scheduler_enqueue(struct thread *t)
{
    t->state = THREAD_READY;

    if (!ready_queue) {
        /* First thread in the queue: it points to itself, forming a
         * one-element circle. */
        ready_queue = t;
        t->next = t;
        return;
    }

    /* Insert immediately before ready_queue (i.e. at the "end" of
     * the rotation, so newly-enqueued threads get their turn after
     * everything already waiting -- this is what makes it FIFO
     * within the round-robin rotation rather than always cutting to
     * the front). */
    struct thread *tail = ready_queue;
    while (tail->next != ready_queue) {
        tail = tail->next;
    }
    tail->next = t;
    t->next = ready_queue;
}

/* Removes and returns the thread at the front of the ready queue,
 * advancing ready_queue to the next thread in rotation. Does NOT
 * change the removed thread's ->next pointer -- callers that intend
 * to keep the thread out of the queue (e.g. because it's about to
 * become `current`) don't need it cleared; callers that need it
 * cleared for correctness do so themselves. */
static struct thread *dequeue_next(void)
{
    if (!ready_queue) {
        return NULL;
    }

    struct thread *t = ready_queue;

    if (t->next == t) {
        /* Only element in the circle. */
        ready_queue = NULL;
    } else {
        struct thread *tail = t;
        while (tail->next != t) {
            tail = tail->next;
        }
        tail->next = t->next;
        ready_queue = t->next;
    }

    return t;
}

struct thread *scheduler_current(void)
{
    return current;
}

/* Performs the actual switch: picks the next ready thread, updates
 * bookkeeping, and calls into context_switch (or
 * thread_prepare_first_switch + context_switch, for a thread's
 * first-ever turn). `from` is the thread being switched away from,
 * which the caller has already updated the state of (READY if it's
 * simply yielding its turn, BLOCKED/DEAD if it can't run again yet)
 * -- this function does not itself decide or set `from`'s state,
 * keeping "why are we switching" (caller's decision) separate from
 * "how do we switch" (this function's job). */
static void switch_to_next(struct thread *from)
{
    struct thread *next = dequeue_next();

    if (!next) {
        /* Nothing else is ready. If `from` itself is still READY
         * (a solo thread yielding to itself, or the idle thread
         * with nothing else to do), just keep running it --
         * re-enqueue and immediately dequeue is equivalent to a
         * no-op switch, so skip the round-trip entirely. */
        if (from && from->state == THREAD_READY) {
            current = from;
            ticks_remaining = SCHEDULER_TIME_SLICE_TICKS;
            return;
        }
        panic("scheduler: ready queue empty and no runnable thread -- system deadlocked");
    }

    struct thread *prev = current;
    current = next;
    current->state = THREAD_RUNNING;
    ticks_remaining = SCHEDULER_TIME_SLICE_TICKS;

    bool next_is_first_run = !next->has_run;

    if (next_is_first_run) {
        thread_prepare_first_switch(next);
        next->has_run = true;
    }

    if (prev) {
        context_switch(&prev->context, &next->context);
    } else {
        /* No previous thread at all -- this is scheduler_start's
         * very first switch. There is nothing to save FROM, so we
         * need a disposable context to satisfy context_switch's
         * signature without corrupting anything real. */
        struct thread_context throwaway;
        context_switch(&throwaway, &next->context);
    }

    /* CRITICAL: unconditionally re-enable interrupts here.
     *
     * Root cause this works around: hardware IRQs (timer, keyboard)
     * are wired through IDT_GATE_INTERRUPT entries (see
     * interrupts.c), which the x86_64 architecture defines as
     * automatically clearing EFLAGS.IF on entry -- this is correct
     * and intentional, preventing a second IRQ from interrupting
     * the handler reentrantly. IF is normally restored afterward by
     * IRETQ, which reloads it from the RFLAGS value the CPU saved
     * on the way in.
     *
     * But when an IRQ handler triggers a context switch (as the
     * timer's does, via scheduler_tick), context_switch's `ret`
     * diverts execution into a COMPLETELY DIFFERENT thread's call
     * stack -- isr_common_stub's IRETQ for THIS interrupt is never
     * reached, because the thread that owned that call stack is no
     * longer the one executing. Whichever thread we just switched
     * INTO resumes with IF still cleared from that original IRQ
     * entry, and stays that way forever: no timer tick can ever
     * fire again to un-stick it, since firing a timer tick is
     * itself gated on IF being set.
     *
     * This was a real bug caught during scheduler bring-up:
     * exactly one preemption occurred (idle -> demo-a), then the
     * timer went completely silent -- confirmed via QEMU's `info
     * registers`, which showed RFL with the IF bit (0x200) cleared
     * on the running thread. Calling sti() here, every time control
     * returns from context_switch regardless of why, guarantees IF
     * is correctly set before any thread resumes -- whether it
     * arrived here via a yield (where IF was already 1, so this is
     * a harmless no-op) or via an interrupt-triggered switch (where
     * this is the only remaining path back to IF=1, since IRETQ for
     * the original interrupt will never execute). */
    sti();
}

static void wake_sleeping_threads(void)
{
    u64 now = timer_ticks();
    struct sleeping_thread **prev = &sleeping_threads;

    while (*prev) {
        if ((*prev)->wake_tick <= now) {
            struct sleeping_thread *node = *prev;
            *prev = node->next;
            if (node->thread->state == THREAD_BLOCKED) {
                scheduler_enqueue(node->thread);
            }
            kfree(node);
            continue;
        }
        prev = &(*prev)->next;
    }
}

void scheduler_tick(void)
{
    if (!scheduler_running || !current) {
        return;
    }

    wake_sleeping_threads();

    if (ticks_remaining > 0) {
        ticks_remaining--;
    }

    if (ticks_remaining == 0) {
        struct thread *prev = current;
        if (prev->state == THREAD_RUNNING) {
            prev->state = THREAD_READY;
            scheduler_enqueue(prev);
            /* scheduler_enqueue sets state back to THREAD_READY
             * redundantly (it already is), which is fine -- keeping
             * enqueue's own invariant ("anything I enqueue, I mark
             * READY") simple and unconditional is worth one
             * redundant store here. */
        }
        switch_to_next(prev);
    }
}

void scheduler_yield(void)
{
    if (!scheduler_running || !current) {
        return;
    }

    struct thread *prev = current;
    if (prev->state == THREAD_RUNNING) {
        prev->state = THREAD_READY;
        scheduler_enqueue(prev);
    }
    switch_to_next(prev);
}

void scheduler_reschedule(void)
{
    if (!scheduler_running || !current) {
        panic("scheduler_reschedule called before scheduler_start");
    }

    /* Unlike scheduler_yield, this does NOT re-enqueue `current` --
     * callers of scheduler_reschedule have already set its state to
     * BLOCKED or DEAD themselves (see the contract in
     * scheduler.h), meaning it must NOT go back into the ready
     * rotation right now. */
    switch_to_next(current);
}

void scheduler_sleep(u64 wake_tick)
{
    struct thread *self = current;
    if (!self) {
        return;
    }

    if (wake_tick <= timer_ticks()) {
        return;
    }

    struct sleeping_thread *entry = kmalloc(sizeof(*entry));
    if (!entry) {
        panic("scheduler_sleep: out of memory");
    }

    entry->thread = self;
    entry->wake_tick = wake_tick;
    entry->next = sleeping_threads;
    sleeping_threads = entry;

    self->state = THREAD_BLOCKED;
    scheduler_reschedule();
}

__attribute__((noreturn)) void scheduler_start(void)
{
    scheduler_running = true;
    switch_to_next(NULL);

    /* switch_to_next never returns on the very first call in
     * practice (it context-switches into some other thread's stack
     * entirely), but the compiler cannot know that from this
     * function's signature alone. Panic rather than silently return
     * into undefined behavior if that assumption is ever violated
     * by a future change. */
    panic("scheduler_start: switch_to_next returned unexpectedly");
}
