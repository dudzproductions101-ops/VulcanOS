















#ifndef VULCAN_PROC_SCHEDULER_H
#define VULCAN_PROC_SCHEDULER_H

#include "types.h"
#include "proc/thread.h"






#define SCHEDULER_TIME_SLICE_TICKS 5

void scheduler_init(void);




void scheduler_enqueue(struct thread *t);






void scheduler_tick(void);





void scheduler_yield(void);






void scheduler_sleep(u64 wake_tick);



struct thread *scheduler_current(void);






void scheduler_reschedule(void);





__attribute__((noreturn)) void scheduler_start(void);

#endif 
