
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

typedef unsigned long      uptr;
typedef signed long        iptr;
typedef unsigned long      usize;
typedef signed long        isize;

#define NULL ((void *)0)

typedef u8 bool;
#define true  1
#define false 0

typedef u64 paddr_t;
typedef u64 vaddr_t;

#endif
