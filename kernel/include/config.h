/*
 * config.h - Compile-time configuration for the VulcanOS kernel
 *
 * Centralizes tunables that would otherwise be scattered as magic
 * numbers across subsystems. Subsystems should #include this rather
 * than hardcoding their own limits.
 */

#ifndef VULCAN_CONFIG_H
#define VULCAN_CONFIG_H

/* Kernel identity, surfaced by the version syscall and by vulcaninfo. */
#define VULCAN_OS_NAME     "VulcanOS"
#define VULCAN_VERSION_MAJ 0
#define VULCAN_VERSION_MIN 1
#define VULCAN_VERSION_PATCH 0
#define VULCAN_CODENAME    "Ember"

/* Kernel virtual address where the higher half begins. VulcanOS maps
 * the kernel into the top of the address space so that user-space
 * mappings (0x0 upward) can never collide with kernel mappings. */
#define VULCAN_KERNEL_VBASE 0xFFFFFFFF80000000UL

/* Size, in bytes, of the kernel's early boot stack. Provided in
 * boot.asm; declared here so both assembly and C agree on the value
 * if it ever needs to change.
 *
 * 128 KiB, not the original 16 KiB: increased after a real, MEASURED
 * stack overflow was found during vpkg bring-up, not guessed at.
 * kmain and everything it calls synchronously before
 * scheduler_start() (fs_bringup, libc_selftest, vpkg_selftest, and
 * everything vpkg_selftest calls transitively -- vpkg_install,
 * vpk_manifest_parse, install_one_file, ...) all share this ONE
 * boot-time stack -- a completely different, much smaller stack
 * than the 64 KiB per-thread kernel stacks (see proc/thread.h) that
 * exist once the scheduler actually starts. Direct RSP measurement
 * (comparing kmain's entry RSP against install_one_file's, several
 * calls deep) showed 23,392 bytes already consumed against the old
 * 16,384-byte stack -- a confirmed 7,008-byte overflow past the end
 * of allocated stack memory, silently corrupting whatever .bss data
 * happened to sit adjacent to it (in VulcanOS's case, this is
 * exactly why the pmm bitmap and other early .bss structures are
 * placed where they are in boot.asm's memory layout -- the overflow
 * was writing into memory that had OTHER purposes, causing symptoms
 * that appeared in unrelated code paths, like corrupted format
 * string pointers in later printk calls). 128 KiB gives real
 * headroom above the measured 23+ KiB single-point requirement,
 * not just barely enough to clear the last observed failure --
 * matching the same "fix it with headroom, not to the exact edge of
 * the last measurement" approach already used for the scheduler's
 * per-thread stack sizing bug. */
#define VULCAN_BOOT_STACK_SIZE (128 * 1024)

/* Maximum number of CPUs VulcanOS's early SMP bring-up will support.
 * Chosen conservatively for the first bring-up milestone; revisit
 * once per-CPU data structures exist. */
#define VULCAN_MAX_CPUS 32

#endif /* VULCAN_CONFIG_H */
