/*
 * panic.h - Unrecoverable kernel error handling
 */

#ifndef VULCAN_PANIC_H
#define VULCAN_PANIC_H

/* Prints a diagnostic message and halts the system permanently.
 * Never returns; callers do not need to handle a return path.
 * Marked noreturn so GCC can warn if code after panic() is
 * unreachable-but-assumed-reachable elsewhere. */
__attribute__((noreturn)) void panic(const char *msg);

#endif /* VULCAN_PANIC_H */
