/*
 * syscall.h - VulcanOS userland process entry-point contract
 *
 * NOT a syscall ABI. This file's name matches the placeholder
 * already anticipated in the original project skeleton, but a real
 * syscall numbering scheme, calling convention, and trap mechanism
 * do not exist yet -- VulcanOS has no ring-3 execution (see the
 * interim-design note in libc/stdlib.c for the full explanation).
 * Inventing a fake syscall ABI now, before there's a real privilege
 * boundary to define it against, would mean throwing it away and
 * redesigning it once that boundary actually exists -- so this file
 * deliberately does NOT do that.
 *
 * What this file DOES define: the entry-point signature every
 * VulcanOS userland program (init, vulsh, ls, cat, echo) follows.
 * Every one of these is currently a real C function, compiled into
 * the kernel image and started as its own scheduler thread (see
 * proc/thread.h's thread_create) -- not a separate executable
 * loaded from disk into an isolated address space. This is an
 * honest reflection of VulcanOS's current architecture, not a
 * lesser version of "real" userland: it follows the same pattern
 * already used successfully for the scheduler bring-up's demo
 * threads (kernel/core/kernel.c), just organized as real,
 * standalone programs instead of inline bring-up scaffolding.
 *
 * When VulcanOS gains real ring-3 execution and an exec()-from-disk
 * mechanism, this file is exactly where the real syscall
 * declarations (open, read, write, fork, exec, wait, ...) belong --
 * the entry-point contract below is written to still make sense on
 * the other side of that transition.
 */

#ifndef VULCAN_USER_SYSCALL_H
#define VULCAN_USER_SYSCALL_H

/* Every VulcanOS userland program's entry point matches this shape:
 * argc/argv, an int return code (0 = success, matching the standard
 * Unix convention any future real process-exit-status mechanism
 * will also want to preserve). Named vulcan_main rather than main
 * to avoid colliding with the freestanding kernel build's own
 * expectations about what "main" means in this translation unit,
 * and because these are not, yet, hosted C programs with a real
 * libc-provided _start -- see thread.h's entry_point field, which is
 * what actually calls into a thin per-program wrapper that adapts
 * vulcan_main's argc/argv signature to thread_create's void(void)
 * entry-point contract (see each program's own _thread_entry
 * wrapper, e.g. user/bin/ls.c). */
typedef int (*vulcan_main_fn)(int argc, char **argv);

#endif /* VULCAN_USER_SYSCALL_H */
