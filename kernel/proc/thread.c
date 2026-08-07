/*
 * thread.c - Thread lifecycle management
 *
 * See proc/thread.h for the context-switch design and
 * proc/thread_switch.asm for the mechanism this file sets up for.
 */

#include "proc/thread.h"
#include "proc/process.h"
#include "mm/allocator.h"
#include "arch/x86_64/cpu.h"
#include "panic.h"

static u64 next_tid = 1;

/* Set by thread_create immediately before the first context_switch
 * into a brand-new thread, read once by thread_trampoline below.
 * This exists because thread_trampoline takes no arguments (it
 * can't -- it's reached via `ret`, not `call`, so there's no
 * argument-passing convention available) but still needs to know
 * which thread and entry point it's starting. A single static
 * works because VulcanOS is single-CPU at this bring-up milestone;
 * this becomes a genuine bug on real SMP (two CPUs could both be
 * mid-first-switch simultaneously) and must become per-CPU state
 * before scheduler.c's design is extended to multiple cores -- see
 * the multicore note in the project roadmap. */
static struct thread *current_thread_being_started = NULL;

/* The real entry point every new thread's first context_switch
 * lands in -- thread_create arranges this by planting this
 * function's address as the "return address" at the top of the
 * thread's freshly-allocated kernel stack (see thread_create
 * below). Routing every new thread through one common trampoline,
 * rather than jumping directly to each thread's own entry_point,
 * gives VulcanOS one place to handle "a thread's entry point
 * returned normally" -- currently that just means the thread is
 * done and should halt rather than execute whatever garbage
 * follows in memory; once the scheduler exists (scheduler.c) this
 * becomes "mark the thread DEAD and reschedule" instead. */
static void thread_trampoline(void)
{
    struct thread *self = current_thread_being_started;
    void (*entry)(void) = self->entry_point;

    /* THE ACTUAL FIX for a real bug found during scheduler bring-up:
     * every new thread's first-ever run is reached via
     * context_switch's bare `ret` landing HERE, not via a normal
     * function return back into switch_to_next (scheduler.c). This
     * matters because hardware IRQs enter through IDT_GATE_INTERRUPT
     * gates, which the CPU defines as automatically clearing
     * EFLAGS.IF on entry -- normally restored by IRETQ on the way
     * out, but IRETQ for the interrupt that triggered a context
     * switch never executes, since the switch diverts execution
     * into a different call stack entirely (see thread_switch.asm
     * and switch_to_next's own comments for the full mechanism).
     *
     * An earlier attempt fixed this by calling sti() immediately
     * after context_switch() returns inside switch_to_next -- which
     * is correct ONLY for a thread being resumed via a normal
     * function return (i.e. a thread that has run before and is now
     * being switched back in). It does nothing for a thread's very
     * FIRST run, because that path never returns to switch_to_next
     * at all -- it lands here instead. Since demo_thread_b (and
     * every other thread) reaches its first busy-wait loop via
     * exactly this path, that first fix left IF permanently cleared
     * for the entire duration of a new thread's first execution --
     * which is why no second thread ever got preempted into: it
     * never had a chance, running with interrupts disabled from the
     * moment it started. */
    sti();

    entry();

    /* entry() returned instead of looping forever or calling an
     * explicit exit primitive. Mark this thread DEAD; the scheduler
     * (once it owns this transition) will stop giving it turns and
     * reclaim it. Until scheduler.c exists to do that reclamation,
     * halting is the safe fallback -- better than falling through
     * into undefined memory. */
    self->state = THREAD_DEAD;
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

struct thread *thread_create(struct process *owner, void (*entry_point)(void))
{
    struct thread *t = kmalloc(sizeof(struct thread));
    if (!t) {
        panic("thread_create: out of memory allocating thread struct");
    }

    t->tid = next_tid++;
    t->state = THREAD_READY;
    t->owner = owner;
    t->entry_point = entry_point;
    t->has_run = false;
    t->next = NULL;

    t->kernel_stack = kmalloc(KERNEL_STACK_SIZE);
    if (!t->kernel_stack) {
        panic("thread_create: out of memory allocating kernel stack");
    }
    t->kernel_stack_top = t->kernel_stack + KERNEL_STACK_SIZE;

    /* Build the initial stack frame context_switch's `ret` expects:
     * a return address at the very top of the stack, exactly as if
     * this thread had previously called context_switch itself and
     * were now returning from it. Since that call never actually
     * happened, we plant thread_trampoline's address there
     * ourselves -- the first context_switch into this thread will
     * `ret` straight into it. */
    u64 *stack_top = (u64 *)t->kernel_stack_top;
    stack_top--;
    *stack_top = (u64)thread_trampoline;

    t->context.rsp = (u64)stack_top;
    t->context.rbx = 0;
    t->context.rbp = 0;
    t->context.r12 = 0;
    t->context.r13 = 0;
    t->context.r14 = 0;
    t->context.r15 = 0;

    return t;
}

void thread_destroy(struct thread *t)
{
    if (!t) {
        return;
    }
    kfree(t->kernel_stack);
    kfree(t);
}

void thread_prepare_first_switch(struct thread *t)
{
    current_thread_being_started = t;
}
