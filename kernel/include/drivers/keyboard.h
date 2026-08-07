/*
 * keyboard.h - PS/2 keyboard driver
 *
 * Polls/handles IRQ1 from the legacy PS/2 controller (8042) and
 * translates scancode set 1 into ASCII. USB HID keyboard support
 * (via a real driver framework) is future work; PS/2 is what every
 * x86 emulator and most real hardware still expose either natively
 * or through USB legacy emulation, so it is the correct first
 * target for a bring-up milestone.
 */

#ifndef VULCAN_DRIVERS_KEYBOARD_H
#define VULCAN_DRIVERS_KEYBOARD_H

#include "types.h"

void keyboard_init(void);

/* Returns the next available key as ASCII, or 0 if the input buffer
 * is empty. Non-blocking by design so callers (e.g. a future shell)
 * decide their own wait policy rather than the driver imposing one. */
char keyboard_read(void);

#endif /* VULCAN_DRIVERS_KEYBOARD_H */
