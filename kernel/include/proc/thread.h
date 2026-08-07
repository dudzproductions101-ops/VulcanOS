












#ifndef VULCAN_PROC_THREAD_H
#define VULCAN_PROC_THREAD_H

#include "types.h"



































#define KERNEL_STACK_SIZE (64 * 1024)

enum thread_state {
    THREAD_READY,      
    THREAD_RUNNING,     
    THREAD_BLOCKED,     
    THREAD_DEAD,        
};














struct thread_context {
    u64 rbx;
    u64 rbp;
    u64 r12;
    u64 r13;
    u64 r14;
    u64 r15;
    u64 rsp;    
};

struct process; 

struct thread {
    u64 tid;
    enum thread_state state;
    struct thread_context context;

    u8 *kernel_stack;          
    u8 *kernel_stack_top;      

    void (*entry_point)(void); 

    bool has_run;               








    struct process *owner;     

    struct thread *next;       

};







struct thread *thread_create(struct process *owner, void (*entry_point)(void));







void thread_destroy(struct thread *t);








void context_switch(struct thread_context *from, struct thread_context *to);









void thread_prepare_first_switch(struct thread *t);

#endif 
