
















#ifndef VULCAN_PROC_PROCESS_H
#define VULCAN_PROC_PROCESS_H

#include "types.h"

#define PROCESS_NAME_MAX 32
#define PROCESS_MAX_THREADS 8

enum process_state {
    PROCESS_ALIVE,
    PROCESS_EXITED,
};

struct thread; 

struct process {
    u64 pid;
    char name[PROCESS_NAME_MAX];
    enum process_state state;
    int exit_code;

    paddr_t page_table_root;   



    struct thread *threads[PROCESS_MAX_THREADS];
    int thread_count;

    struct process *next;      




};





struct process *process_create(const char *name, void (*entry_point)(void));







void process_exit(struct process *p, int exit_code);

#endif 
