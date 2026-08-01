
#ifndef VULCAN_DRIVERS_TIMER_H
#define VULCAN_DRIVERS_TIMER_H

#include "types.h"

#define PIT_FREQUENCY_HZ 100

void timer_init(void);
u64 timer_ticks(void);

#endif
