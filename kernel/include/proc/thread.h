/*
 * thread.h - Thread control block and context-switch primitives
 *
 * VulcanOS splits process and thread deliberately, matching the
 * standard Unix separation: a process (process.h) owns an address
 * space and resources; a thread is one schedulable unit of
 * execution within that address space. Every process has at least
 * one thread; this bring-up milestone only exercises the
 * one-thread-per-process case, but the split is real from the start
 * rather than bolted on later, since retrofitting multi-threading
 * onto a process-only design tends to leak assumptions everywhere.
 */

#ifndef VULCAN_PROC_THREAD_H
#define VULCAN_PROC_THREAD_H

#include "types.h"

/* 64 KiB per thread, not 16 KiB -- increased during scheduler
 * bring-up after a real stack-exhaustion bug was found and traced
 * precisely (not guessed): calling sti() inside thread_trampoline
 * (thread.c) re-enables interrupts while still structurally
 * "inside" the original interrupt-handler call chain that triggered
 * the switch into this thread (isr_common_stub -> isr_dispatch ->
 * timer_irq_handler -> scheduler_tick -> switch_to_next ->
 * context_switch -> thread_trampoline) -- none of those frames have
 * unwound, they're paused underneath. A subsequent interrupt then
 * pushes an entirely new frame set on top of all of them, and if
 * THAT handler also switches, the nesting compounds again, with no
 * guaranteed unwind point. Confirmed via QEMU's -d int trace: two
 * timer interrupts serviced normally, then a third interrupt
 * (keyboard, IRQ1) faulted immediately on entry with a visibly
 * corrupted RSP (repeating garbage pattern), cascading through a
 * GPF -> double fault -> triple fault -- consistent with the stack
 * having been exhausted by unbounded nested interrupt frames rather
 * than a single bad pointer write.
 *
 * 64 KiB is a STOPGAP, not the real fix: it makes the failure
 * happen at a much deeper nesting level, which is enough headroom
 * for this bring-up milestone's light interrupt load, but does not
 * bound nesting depth at all -- a sufficiently adversarial interrupt
 * pattern could still exhaust even 64 KiB. The architecturally
 * correct fix is ensuring an interrupt's own isr_common_stub
 * always reaches its iretq (fully unwinding that specific
 * interrupt's frames) before a subsequent interrupt is allowed to
 * nest on the same stack -- e.g. via a per-CPU "currently servicing
 * an interrupt" depth counter that scheduler_tick consults before
 * deciding to switch immediately vs. deferring the switch until a
 * safe unwind point, or via a dedicated interrupt stack (IST) with
 * bounded, known depth instead of sharing each thread's own kernel
 * stack for interrupt frames at all. This is real, tracked future
 * work -- not resolved by this stack-size increase alone. */
#define KERNEL_STACK_SIZE (64 * 1024)

enum thread_state {
    THREAD_READY,      /* runnable, waiting for the scheduler to pick it */
    THREAD_RUNNING,     /* currently on the CPU */
    THREAD_BLOCKED,     /* waiting on something (I/O, a lock, ...) */
    THREAD_DEAD,        /* finished; resources not yet reclaimed */
};

/* Callee-saved general-purpose registers plus the stack pointer,
 * per the System V AMD64 ABI: rbx, rbp, r12-r15 must survive a
 * function call unless the callee explicitly saves/restores them
 * itself. A context switch IS exactly that kind of call boundary,
 * so only these registers (plus rsp/rip, handled implicitly by the
 * switch mechanism) need saving -- caller-saved registers (rax,
 * rcx, rdx, rsi, rdi, r8-r11) are already the calling code's
 * responsibility to preserve across any call, context switch
 * included. This is why context_switch (thread_switch.asm) is
 * smaller than the interrupt_frame struct in interrupts.h: an
 * interrupt can occur at any instruction and must save everything,
 * but a context switch happens at a controlled call site.
 */
struct thread_context {
    u64 rbx;
    u64 rbp;
    u64 r12;
    u64 r13;
    u64 r14;
    u64 r15;
    u64 rsp;    /* saved last: switch_to reads/writes this specially */
};

struct process; /* forward declaration; full definition in process.h */

struct thread {
    u64 tid;
    enum thread_state state;
    struct thread_context context;

    u8 *kernel_stack;          /* base of this thread's allocated kernel stack */
    u8 *kernel_stack_top;      /* the address rsp starts at (stack grows down) */

    void (*entry_point)(void); /* read once by thread_trampoline (thread.c) on
                                 * this thread's first-ever context switch */
    bool has_run;               /* false until this thread's first context
                                 * switch has happened; see scheduler.c's
                                 * switch_to_next for why this can't be
                                 * inferred from context.rsp instead (rsp is
                                 * set to a real, nonzero stack address at
                                 * creation time, before the thread has ever
                                 * actually run, so rsp==0 is never a valid
                                 * "hasn't run yet" signal) */

    struct process *owner;     /* the process this thread belongs to */

    struct thread *next;       /* intrusive singly-linked list, used by the
                                 * scheduler's ready queue (see scheduler.c) */
};

/* Allocates a thread control block and its kernel stack, and points
 * its saved context at entry_point such that the first context
 * switch into this thread begins executing there. Does NOT add the
 * thread to the scheduler's ready queue -- callers do that
 * explicitly via scheduler_enqueue, keeping "construct a thread" and
 * "make it runnable" separate operations. */
struct thread *thread_create(struct process *owner, void (*entry_point)(void));

/* Frees a thread's kernel stack and control block. Caller must
 * ensure the thread is not currently running and not on any
 * scheduler queue -- thread_destroy performs no such checks itself,
 * since by the time something is confident enough to call destroy,
 * it already holds that responsibility (typically the scheduler's
 * reaping path, which removes from queues before destroying). */
void thread_destroy(struct thread *t);

/* Implemented in thread_switch.asm. Saves the CURRENT thread's
 * callee-saved registers and stack pointer into `from`, then
 * restores `to`'s, and returns -- but "returns" here means resuming
 * inside whatever call originally invoked switch_to for `to`, which
 * on a thread's very first run is a synthetic return address
 * planted by thread_create pointing at thread_trampoline instead of
 * a real prior call site. */
void context_switch(struct thread_context *from, struct thread_context *to);

/* Records which thread is about to receive its first-ever
 * context_switch, so thread_trampoline (thread.c) knows which
 * thread's entry_point to call once it's reached via `ret`. Must be
 * called by the scheduler immediately before context_switch when,
 * and only when, `to` has never run before (i.e. this is its first
 * turn) -- see the single-CPU caveat in thread.c's
 * current_thread_being_started comment for why this doesn't
 * generalize to SMP as written. */
void thread_prepare_first_switch(struct thread *t);

#endif /* VULCAN_PROC_THREAD_H */
