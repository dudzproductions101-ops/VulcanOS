/*
 * init.c - VulcanOS's init process
 *
 * The first real userland process the scheduler starts (PID 2,
 * right after the idle thread) -- replaces the ad-hoc demo_thread_a/
 * demo_thread_b bring-up scaffolding that proved the scheduler
 * itself worked (kernel/core/kernel.c still keeps those functions
 * defined, undisturbed, as documented scheduler-verification code;
 * they're simply no longer what boots by default). init's job, for
 * this bring-up milestone, is the honest minimum a real init would
 * do first: start the shell. A real init would also parse a service
 * manifest, supervise/restart crashed services, and eventually
 * matches the project's stated "beginner-friendly install, powerful
 * CLI" goals -- all reasonable future work once there's more than
 * one userland program worth supervising.
 *
 * ADAPTER NOTE: process_create/thread_create expect a void(void)
 * entry point (proc/thread.h), but real userland programs follow
 * vulcan_main_fn's argc/argv shape (user/include/syscall.h). init
 * is where this adapter first has to exist for real: init_thread_entry
 * is the void(void) thread_create sees; it builds a minimal argv
 * (just the program name) and calls vulsh_main with it directly,
 * since init doesn't fork+exec vulsh as a separate thread -- it IS
 * the same thread, just handing off to the shell's own main loop.
 */

#include "init.h"
#include "shell.h"
#include "stdio.h"

void init_thread_entry(void)
{
    printf("init: VulcanOS userland starting.\n");

    char *argv[] = { "vulsh" };
    vulsh_main(1, argv);

    /* vulsh_main only returns if its own loop somehow exits without
     * going through exit() -- not expected in normal operation, but
     * if it happens, that's init's own thread finishing normally,
     * which thread.c's trampoline already handles correctly (marks
     * the thread DEAD, see thread_trampoline's contract). No
     * explicit exit() call needed here for that path. */
    printf("init: shell exited unexpectedly.\n");
}
