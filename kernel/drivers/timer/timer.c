#include "drivers/timer.h"
#include "arch/x86_64/interrupts.h"
#include "arch/x86_64/cpu.h"
#include "proc/scheduler.h"

#define PIT_CHANNEL0_DATA 0x40
#define PIT_COMMAND       0x43
#define PIT_BASE_FREQ_HZ  1193182  

static volatile u64 ticks = 0;

static void timer_irq_handler(struct interrupt_frame *frame)
{
    (void)frame;
    ticks++;

    scheduler_tick();
}

void timer_init(void)
{
    u16 divisor = (u16)(PIT_BASE_FREQ_HZ / PIT_FREQUENCY_HZ);

    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL0_DATA, (u8)(divisor & 0xFF));
    outb(PIT_CHANNEL0_DATA, (u8)((divisor >> 8) & 0xFF));

    irq_register_handler(0, timer_irq_handler);
}

u64 timer_ticks(void)
{
    return ticks;
}
