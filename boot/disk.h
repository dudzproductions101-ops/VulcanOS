#ifndef DISK_H
#define DISK_H

#include <stdint.h>

struct disk {
    const char *name;
    uint64_t size;
};

struct disk_partition {
    const char *name;
    uint64_t size;
};

struct disk *disk_get(void);
struct disk_partition *disk_get_partition(struct disk *disk, int index);

void disk_init(void);

#endif