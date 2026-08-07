#ifndef VULCAN_DRIVERS_BLOCK_DEV_H
#define VULCAN_DRIVERS_BLOCK_DEV_H

#include "types.h"

typedef isize (*block_read_fn)(void *ctx, u64 offset, void *buf, usize size);
typedef isize (*block_write_fn)(void *ctx, u64 offset, const void *buf, usize size);

bool block_device_register(const char *name, u64 size_bytes, void *ctx,
                           block_read_fn read_fn,
                           block_write_fn write_fn);
bool block_device_get_report(const char *name, u64 *out_size_bytes, u32 *out_pref_block);

usize block_device_count(void);
const char *block_device_name(usize idx);

isize block_device_read_by_name(const char *name, u64 offset, void *buf, usize size);
isize block_device_write_by_name(const char *name, u64 offset, const void *buf, usize size);

#endif
