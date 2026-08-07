/*
 * process.h - Process control block
 *
 * A process is an address space plus bookkeeping (PID, name,
 * exit status) plus a list of threads that execute within that
 * address space. This bring-up milestone's processes are entirely
 * kernel-space: every process shares the kernel's own page tables
 * (see mm/paging.c's kernel_pml4) rather than getting a private
 * address space, since user-mode execution needs ring-3 transition
 * machinery (a real TSS.rsp0, syscalls, user-accessible page
 * mappings) that doesn't exist yet. The `page_table_root` field
 * below is deliberately present now, even though every process
 * currently sets it to the same kernel PML4 -- so that adding real
 * per-process address spaces later is "populate this field
 * correctly" rather than a structural change to process.h itself.
 */

#ifndef VULCAN_PROC_PROCESS_H
#define VULCAN_PROC_PROCESS_H

#include "types.h"

#define PROCESS_NAME_MAX 32
#define PROCESS_MAX_THREADS 8

enum process_state {
    PROCESS_ALIVE,
    PROCESS_EXITED,
};

struct thread; /* forward declaration; full definition in thread.h */

struct process {
    u64 pid;
    char name[PROCESS_NAME_MAX];
    enum process_state state;
    int exit_code;

    paddr_t page_table_root;   /* CR3 value for this process; see file
                                 * comment above re: currently always the
                                 * kernel's own PML4 */

    struct thread *threads[PROCESS_MAX_THREADS];
    int thread_count;

    struct process *next;      /* intrusive list of all live processes,
                                 * used for bookkeeping/debugging (e.g. a
                                 * future `ps`), NOT the scheduler's ready
                                 * queue -- that's threads, tracked in
                                 * scheduler.c */
};

/* Creates a process control block and its first thread, which will
 * begin executing at entry_point once scheduled. Does not enqueue
 * that thread with the scheduler -- see thread_create's comment in
 * thread.h for why construction and scheduling are kept separate. */
struct process *process_create(const char *name, void (*entry_point)(void));

/* Marks a process exited and records its exit code. Does not
 * immediately destroy the process's threads or reclaim memory --
 * matching the Unix convention that an exited-but-not-yet-reaped
 * process (a "zombie") remains inspectable until something collects
 * its exit status. Actual reaping is scheduler/init policy, not
 * this function's job. */
void process_exit(struct process *p, int exit_code);

#endif /* VULCAN_PROC_PROCESS_H */
