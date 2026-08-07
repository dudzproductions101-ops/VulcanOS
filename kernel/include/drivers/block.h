#ifndef VULCAN_DRIVERS_BLOCK_H
#define VULCAN_DRIVERS_BLOCK_H

#include "types.h"

bool ramdisk_available(void);
u64 ramdisk_size_bytes(void);
isize ramdisk_read(u64 offset, void *buf, usize size);

bool ramdisk_driver_register(void);

#endif 
