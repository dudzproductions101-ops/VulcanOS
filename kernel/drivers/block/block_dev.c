#include "drivers/block_dev.h"
#include "mm/allocator.h"
#include "printk.h"
#include "string.h"

#define MAX_BLOCK_DEVICES 8

struct block_dev_entry {
    char name[32];
    u64 size_bytes;
    void *ctx;
    block_read_fn read;
    block_write_fn write;
    bool used;
};

static struct block_dev_entry devices[MAX_BLOCK_DEVICES];

bool block_device_register(const char *name, u64 size_bytes, void *ctx,
                           block_read_fn read_fn,
                           block_write_fn write_fn)
{
    for (int i = 0; i < MAX_BLOCK_DEVICES; i++) {
        if (!devices[i].used) {
            devices[i].used = true;
            int j = 0;
            for (; j < 31 && name[j]; j++) devices[i].name[j] = name[j];
            devices[i].name[j] = '\0';
            devices[i].size_bytes = size_bytes;
            devices[i].ctx = ctx;
            devices[i].read = read_fn;
            devices[i].write = write_fn;
            printk_level(LOG_INFO, "block: registered device %s size=%llu\n", devices[i].name, size_bytes);
            return true;
        }
    }
    printk_level(LOG_WARN, "block: device registry full, cannot register %s\n", name);
    return false;
}

bool block_device_get_report(const char *name, u64 *out_size_bytes, u32 *out_pref_block)
{
    for (int i = 0; i < MAX_BLOCK_DEVICES; i++) {
        if (!devices[i].used) continue;
        if (strcmp(devices[i].name, name) == 0) {
            *out_size_bytes = devices[i].size_bytes;
            *out_pref_block = 512;
            return true;
        }
    }
    return false;
}

usize block_device_count(void)
{
    usize count = 0;
    for (int i = 0; i < MAX_BLOCK_DEVICES; i++) {
        if (devices[i].used) {
            count++;
        }
    }
    return count;
}

const char *block_device_name(usize idx)
{
    usize count = 0;
    for (int i = 0; i < MAX_BLOCK_DEVICES; i++) {
        if (!devices[i].used) continue;
        if (count == idx) {
            return devices[i].name;
        }
        count++;
    }
    return NULL;
}

isize block_device_read_by_name(const char *name, u64 offset, void *buf, usize size)
{
    for (int i = 0; i < MAX_BLOCK_DEVICES; i++) {
        if (!devices[i].used) continue;
        if (strcmp(devices[i].name, name) == 0) {
            if (devices[i].read) return devices[i].read(devices[i].ctx, offset, buf, size);
            return -1;
        }
    }
    return -1;
}

isize block_device_write_by_name(const char *name, u64 offset, const void *buf, usize size)
{
    for (int i = 0; i < MAX_BLOCK_DEVICES; i++) {
        if (!devices[i].used) continue;
        if (strcmp(devices[i].name, name) == 0) {
            if (devices[i].write) return devices[i].write(devices[i].ctx, offset, buf, size);
            return -1;
        }
    }
    return -1;
}
