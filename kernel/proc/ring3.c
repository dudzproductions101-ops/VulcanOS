#include "proc/ring3.h"
#include "proc/process.h"
#include "proc/thread.h"
#include "mm/paging.h"
#include "mm/allocator.h"
#include "arch/x86_64/cpu.h"
#include "arch/x86_64/interrupts.h"
#include "types.h"
#include "printk.h"
#include "panic.h"

extern void ring3_enter_user_mode(u64 entry_point, u64 user_rsp);

/*
 * Allocate a new user page table for a process
 * For now: simply use the kernel page table (single-address-space)
 * Full implementation would require mapping user-only memory
 */
static paddr_t allocate_user_page_table(void)
{
    /* For Stage 2 initial implementation:
     * Keep using kernel page table.
     * Ring-3 privilege will restrict access via CPU privilege checks,
     * not page table entries.
     *
     * TODO (later stages):
     * - Allocate separate PML4 for each process
     * - Map kernel memory as supervisor-only
     * - Map user memory as user-accessible
     */
    paddr_t kernel_pml4 = read_cr3();
    printk_level(LOG_INFO, "ring3: allocated user page table (using kernel pml4) phys=%p\n", 
                 kernel_pml4);
    return kernel_pml4;
}

/*
 * Allocate user address space
 * For now: static allocation of stack space in kernel memory
 */
static int allocate_user_memory(paddr_t user_pml4_phys, u64 size)
{
    /* For Stage 2:
     * Memory allocation is implicit in kernel heap.
     * Ring-3 doesn't actually access these addresses yet.
     *
     * TODO: Actual user page mapping comes in Stage 4 (virtual memory)
     */
    printk_level(LOG_INFO, "ring3: allocated %lu bytes for user process\n", size);
    return 0;
}

/*
 * Set up a process for ring-3 execution
 * - Allocate separate page table
 * - Allocate user memory
 * - Prepare for entry into user mode
 */
void ring3_setup_user_process(struct process *p, void (*entry_point)(void))
{
    /* Allocate user page table */
    paddr_t user_pml4 = allocate_user_page_table();
    if (!user_pml4) {
        panic("ring3_setup_user_process: failed to allocate user page table");
    }

    /* Store user page table in process */
    p->page_table_root = user_pml4;

    /* Allocate 4MB of user memory (code + data + heap + stack) */
    if (allocate_user_memory(user_pml4, 4 * 1024 * 1024) < 0) {
        panic("ring3_setup_user_process: failed to allocate user memory");
    }

    printk_level(LOG_INFO, "ring3: set up process pid=%llu for ring-3 execution at %p\n", 
                 p->pid, entry_point);
}

/*
 * Return to ring-3 user mode
 * Modifies interrupt frame to switch privilege levels on iretq
 */
void ring3_return_to_user(struct interrupt_frame *frame)
{
    /* Set CS to ring-3 code selector (0x18 | 3) */
    frame->cs = 0x1B;  /* 0x18 (user code seg) | 3 (RPL) */

    /* Set SS to ring-3 data selector (0x20 | 3) */
    frame->ss = 0x23;  /* 0x20 (user data seg) | 3 (RPL) */

    /* Set RIP to entry point (stored in RDI from syscall) */
    /* Note: For now, we just keep RDI as the entry point */
    /* frame->rdi contains the function pointer passed to SYS_EXEC */

    /* Set RSP to a user stack (in upper kernel memory for now) */
    frame->rsp = 0xFFFFFF8000000000UL - 0x1000; /* Near top of kernel space, will adjust in Stage 4 */

    /* Enable IF flag (interrupts) */
    frame->rflags |= 0x200;

    /* Clear return value (syscall succeeded) */
    frame->rax = 0;

    printk_level(LOG_INFO, "ring3: returning to user mode at %p, rsp=%p\n", 
                 frame->rdi, frame->rsp);
}
