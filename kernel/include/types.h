/*
 * types.h - VulcanOS fixed-width type definitions
 *
 * Freestanding kernel code cannot rely on the hosted C library, so
 * VulcanOS defines its own minimal set of fixed-width integer types
 * instead of pulling in <stdint.h>. This keeps the kernel's ABI
 * explicit and independent of whatever the host toolchain ships.
 *
 * Part of VulcanOS - originally authored for this project.
 */

#ifndef VULCAN_TYPES_H
#define VULCAN_TYPES_H

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef signed char        i8;
typedef signed short       i16;
typedef signed int         i32;
typedef signed long long   i64;

/* Pointer-sized integers. On x86_64 (LP64-like kernel model) a long
 * is 64 bits, which matches pointer width. */
typedef unsigned long      uptr;
typedef signed long        iptr;
typedef unsigned long      usize;
typedef signed long        isize;

#define NULL ((void *)0)

typedef u8 bool;
#define true  1
#define false 0

/* Physical and virtual address types are kept distinct (even though
 * both are u64) so that function signatures document, at a glance,
 * whether an address has already been mapped into the kernel's
 * virtual address space or still needs to be. */
typedef u64 paddr_t;
typedef u64 vaddr_t;

#endif /* VULCAN_TYPES_H */
