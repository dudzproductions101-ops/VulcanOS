#ifndef VULCAN_SYSCALL_H
#define VULCAN_SYSCALL_H

#include "types.h"
#include "arch/x86_64/interrupts.h"

#define SYS_GETPID 1
#define SYS_WRITE  2
#define SYS_EXIT   3

void syscall_handle(struct interrupt_frame *frame);

#endif 
