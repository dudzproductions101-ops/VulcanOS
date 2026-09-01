#ifndef VULCAN_SYSCALL_H
#define VULCAN_SYSCALL_H

#include "types.h"
#include "arch/x86_64/interrupts.h"

/* File descriptors */
#define VULCAN_STDIN  0
#define VULCAN_STDOUT 1
#define VULCAN_STDERR 2

/* Syscall numbers */
#define SYS_GETPID 1
#define SYS_READ   2
#define SYS_WRITE  3
#define SYS_EXIT   4

/* Graphics syscalls (Stage 1) */
#define SYS_GRAPHICS_INFO       10
#define SYS_GRAPHICS_CLEAR      11
#define SYS_GRAPHICS_DRAW_RECT  12

/* Process management syscalls (Stage 2) */
#define SYS_EXEC  20
#define SYS_WAIT  21
#define SYS_FORK  22
#define SYS_KILL  23

void syscall_handle(struct interrupt_frame *frame);

#endif
