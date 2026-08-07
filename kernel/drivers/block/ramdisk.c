#include "drivers/block.h"
#include "drivers/block_dev.h"
#include "drivers/driver.h"
#include "mm/allocator.h"
#include "printk.h"

#define RAMDISK_DEFAULT_SIZE (4 * 1024 * 1024) /* 4 MiB */

static u8 *ramdisk_buf = NULL;
static u64 ramdisk_size = 0;

bool ramdisk_available(void)
{
    return ramdisk_buf != NULL;
}

u64 ramdisk_size_bytes(void)
{
    return ramdisk_size;
}

isize ramdisk_read(u64 offset, void *buf, usize size)
{
    if (!ramdisk_buf) return -1;
    if (offset >= ramdisk_size) return 0;
    u64 max = ramdisk_size - offset;
    usize to_copy = (size < max) ? size : (usize)max;
    for (usize i = 0; i < to_copy; i++) {
        ((u8 *)buf)[i] = ramdisk_buf[offset + i];
    }
    return (isize)to_copy;
}

static bool ramdisk_probe(void)
{
    /* Always available once allocated in init; probe returns true
     * only when we have a buffer. */
    return ramdisk_available();
}

static isize ramdisk_block_read(void *ctx, u64 offset, void *buf, usize size)
{
    (void)ctx;
    return ramdisk_read(offset, buf, size);
}

static isize ramdisk_block_write(void *ctx, u64 offset, const void *buf, usize size)
{
    (void)ctx;
    if (!ramdisk_buf) {
        return -1;
    }
    if (offset >= ramdisk_size) {
        return 0;
    }
    u64 max = ramdisk_size - offset;
    usize to_copy = (size < max) ? size : (usize)max;
    for (usize i = 0; i < to_copy; i++) {
        ramdisk_buf[offset + i] = ((const u8 *)buf)[i];
    }
    return (isize)to_copy;
}

static void ramdisk_init_driver(void)
{
    if (!ramdisk_buf) {
        ramdisk_buf = kmalloc(RAMDISK_DEFAULT_SIZE);
        if (!ramdisk_buf) {
            printk_level(LOG_WARN, "ramdisk: failed to allocate buffer\n");
            ramdisk_size = 0;
            return;
        }
        ramdisk_size = RAMDISK_DEFAULT_SIZE;
        for (u64 i = 0; i < ramdisk_size; i++) {
            ramdisk_buf[i] = 0; /* empty image by default */
        }
        printk_level(LOG_INFO, "ramdisk: allocated %llu bytes\n", ramdisk_size);
        if (!block_device_register("ram0", ramdisk_size, NULL,
                                   ramdisk_block_read, ramdisk_block_write)) {
            printk_level(LOG_WARN, "ramdisk: failed to register block device ram0\n");
        }
    }
}

static const struct driver ramdisk_driver = {
    .name = "ramdisk",
    .class = DRIVER_CLASS_STORAGE,
    .probe = ramdisk_probe,
    .init = ramdisk_init_driver,
    .api = NULL,
};

bool ramdisk_driver_register(void)
{
    return driver_register(&ramdisk_driver);
}
