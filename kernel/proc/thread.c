#include "proc/thread.h"
#include "proc/process.h"
#include "mm/allocator.h"
#include "arch/x86_64/cpu.h"
#include "panic.h"

static u64 next_tid = 1;

static struct thread *current_thread_being_started = NULL;

static void thread_trampoline(void)
{
    struct thread *self = current_thread_being_started;
    void (*entry)(void) = self->entry_point;

    sti();

    entry();

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
