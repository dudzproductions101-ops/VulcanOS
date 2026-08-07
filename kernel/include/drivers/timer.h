/*
 * timer.h - Programmable Interval Timer (PIT) driver
 *
 * The legacy 8253/8254 PIT is the only timer guaranteed present and
 * simple to program without ACPI table parsing, so VulcanOS uses it
 * for the first-boot tick source. It will be superseded by the
 * Local APIC timer once interrupt handling moves past the legacy
 * PIC (see the roadmap note in interrupts.c).
 */

#ifndef VULCAN_DRIVERS_TIMER_H
#define VULCAN_DRIVERS_TIMER_H

#include "types.h"

#define PIT_FREQUENCY_HZ 100  /* 100 ticks/sec = 10ms resolution */

void timer_init(void);
u64 timer_ticks(void);

#endif /* VULCAN_DRIVERS_TIMER_H */
