/*
 * init.h - VulcanOS's init process public interface
 */

#ifndef VULCAN_USER_INIT_H
#define VULCAN_USER_INIT_H

/* The thread-entry-point wrapper process_create expects (see
 * proc/thread.h -- entry_point is void(void), not the argc/argv
 * shape every real userland program's vulcan_main follows; see
 * init.c for the adapter). This is what kernel/core/kernel.c passes
 * to process_create("init", ...) to start the first real userland
 * process. */
void init_thread_entry(void);

#endif /* VULCAN_USER_INIT_H */
