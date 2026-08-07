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

static struct thread *ready_queue = NULL;   
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

        ready_queue = t;
        t->next = t;
        return;
    }

    struct thread *tail = ready_queue;
    while (tail->next != ready_queue) {
        tail = tail->next;
    }
    tail->next = t;
    t->next = ready_queue;
}

static struct thread *dequeue_next(void)
{
    if (!ready_queue) {
        return NULL;
    }

    struct thread *t = ready_queue;

    if (t->next == t) {

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

static void switch_to_next(struct thread *from)
{
    struct thread *next = dequeue_next();

    if (!next) {

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

        struct thread_context throwaway;
        context_switch(&throwaway, &next->context);
    }

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

    panic("scheduler_start: switch_to_next returned unexpectedly");
}
