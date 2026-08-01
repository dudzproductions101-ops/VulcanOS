
#include "stdlib.h"
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

    extern __attribute__((noreturn)) void panic(const char *msg);
    panic("exit: scheduler_reschedule returned for a DEAD thread");
}

void yield(void)
{
    scheduler_yield();
}
