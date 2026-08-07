/* block.h - Block device interface */

#ifndef VULCAN_DRIVERS_BLOCK_H
#define VULCAN_DRIVERS_BLOCK_H

#include "types.h"

/* Minimal block device API used by early filesystem/device code.
 * Implementations live in drivers/block (ramdisk.c) and register via
 * the driver manager so the kernel can discover storage devices. */

bool ramdisk_available(void);
u64 ramdisk_size_bytes(void);
isize ramdisk_read(u64 offset, void *buf, usize size);

/* Registration helper (called from kmain before driver_init_all()). */
bool ramdisk_driver_register(void);

#endif /* VULCAN_DRIVERS_BLOCK_H */
