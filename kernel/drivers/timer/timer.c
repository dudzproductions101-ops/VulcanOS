/*
 * timer.c - Programmable Interval Timer (PIT) driver
 *
 * See drivers/timer.h for why the legacy PIT is the correct choice
 * for this bring-up milestone rather than the Local APIC timer.
 */

#include "drivers/timer.h"
#include "arch/x86_64/interrupts.h"
#include "arch/x86_64/cpu.h"
#include "proc/scheduler.h"

#define PIT_CHANNEL0_DATA 0x40
#define PIT_COMMAND       0x43
#define PIT_BASE_FREQ_HZ  1193182  /* fixed oscillator frequency, not configurable */

static volatile u64 ticks = 0;

static void timer_irq_handler(struct interrupt_frame *frame)
{
    (void)frame;
    ticks++;

    /* scheduler_tick may context-switch away from this exact call
     * stack -- INCLUDING while we're still logically inside this
     * interrupt handler, with isr_common_stub's pushed register
     * frame still sitting further down this thread's kernel stack,
     * unpopped and unreturned-from. This is only safe because every
     * VulcanOS thread has its OWN private kernel stack (see
     * proc/thread.c): the interrupted thread's ISR frame stays
     * exactly where it was left, undisturbed by whatever other
     * thread runs in between, and is correctly popped + iretq'd
     * once a normal function-return chain eventually walks back up
     * to it after this thread is scheduled again. If VulcanOS ever
     * moves to a shared/global interrupt stack (e.g. via IST) for
     * any vector that can reach here, this call becomes unsafe and
     * must be replaced with a deferred "reschedule after this IRQ
     * returns" flag instead of a direct call. */
    scheduler_tick();
}

void timer_init(void)
{
    u16 divisor = (u16)(PIT_BASE_FREQ_HZ / PIT_FREQUENCY_HZ);

    /* 0x36 = channel 0, access lobyte/hibyte, mode 3 (square wave),
     * binary (not BCD) counting -- the standard PIT initialization
     * sequence documented since the original 8253/8254 datasheet. */
    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL0_DATA, (u8)(divisor & 0xFF));
    outb(PIT_CHANNEL0_DATA, (u8)((divisor >> 8) & 0xFF));

    irq_register_handler(0, timer_irq_handler);
}

u64 timer_ticks(void)
{
    return ticks;
}
