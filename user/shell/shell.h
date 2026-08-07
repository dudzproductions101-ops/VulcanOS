/*
 * shell.h - VulcanOS shell (vulsh) public interface
 */

#ifndef VULCAN_USER_SHELL_H
#define VULCAN_USER_SHELL_H

/* The shell's real entry point, called as this thread's argc/argv
 * "main" (see user/include/syscall.h's vulcan_main_fn contract).
 * Never returns under normal operation -- a shell that exits is
 * expected to call exit() itself (e.g. on an "exit" built-in
 * command), matching real shell behavior. */
int vulsh_main(int argc, char **argv);

#endif /* VULCAN_USER_SHELL_H */
