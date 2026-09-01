#ifndef VULCAN_PROC_RING3_H
#define VULCAN_PROC_RING3_H

#include "types.h"
#include "proc/process.h"
#include "arch/x86_64/interrupts.h"

/*
 * Ring-3 User Process Support (Stage 2)
 *
 * This header defines the infrastructure for executing code at privilege level 3 (ring 3).
 * Key concepts:
 * - Each ring-3 process has its own address space and page table
 * - Ring-3 code cannot access kernel memory or I/O ports
 * - Ring-3 processes transition to ring 0 via syscalls
 * - TSS (Task State Segment) maintains kernel stack pointer for transitions
 */

/*
 * User process entry point:
 * All ring-3 processes enter at this address with minimal setup.
 * The kernel loads code/data/BSS from ELF binary into user memory.
 */
#define USER_CODE_ENTRY 0x400000UL

/*
 * User address space layout:
 * 0x400000 - 0x7FFFFFFF: User code/data/heap
 * (above 0x80000000: reserved for kernel)
 */
#define USER_SPACE_START 0x400000UL
#define USER_SPACE_END   0x80000000UL

/*
 * Set up a process for ring-3 execution
 * Allocates user page table, maps code/data at USER_CODE_ENTRY
 * Called before first context switch to the process
 */
void ring3_setup_user_process(struct process *p, void (*entry_point)(void));

/*
 * Switch to ring-3 user process at (addr, privilege level 3)
 * Loads user code/data/stack selectors
 * CPU transitions from ring 0 to ring 3
 * Returns on syscall or interrupt back to ring 0
 */
void ring3_enter_user_mode(u64 entry_point, u64 user_rsp);

/*
 * Return to ring-3 after syscall
 * CPU transitions from ring 0 back to ring 3
 */
void ring3_return_to_user(struct interrupt_frame *frame);

#endif
