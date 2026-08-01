
#include "proc/process.h"
#include "proc/thread.h"
#include "mm/allocator.h"
#include "mm/paging.h"
#include "arch/x86_64/cpu.h"
#include "panic.h"
#include "printk.h"

static u64 next_pid = 1;

static void copy_name(char *dest, const char *src)
{
    int i = 0;
    for (; i < PROCESS_NAME_MAX - 1 && src[i]; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

struct process *process_create(const char *name, void (*entry_point)(void))
{
    struct process *p = kmalloc(sizeof(struct process));
    if (!p) {
        panic("process_create: out of memory allocating process struct");
    }

    p->pid = next_pid++;
    copy_name(p->name, name);
    p->state = PROCESS_ALIVE;
    p->exit_code = 0;
    p->thread_count = 0;
    p->next = NULL;

    p->page_table_root = read_cr3();

    struct thread *t = thread_create(p, entry_point);
    p->threads[0] = t;
    p->thread_count = 1;

    printk_level(LOG_INFO, "process: created pid=%llu name=\"%s\" tid=%llu\n",
                 p->pid, p->name, t->tid);

    return p;
}

void process_exit(struct process *p, int exit_code)
{
    p->state = PROCESS_EXITED;
    p->exit_code = exit_code;

    printk_level(LOG_INFO, "process: pid=%llu \"%s\" exited with code %d\n",
                 p->pid, p->name, exit_code);
}
