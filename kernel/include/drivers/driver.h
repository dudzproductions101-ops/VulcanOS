








#ifndef VULCAN_DRIVERS_DRIVER_H
#define VULCAN_DRIVERS_DRIVER_H

#include "types.h"

enum driver_class {
    DRIVER_CLASS_DISPLAY,
    DRIVER_CLASS_INPUT,
    DRIVER_CLASS_STORAGE,
    DRIVER_CLASS_OTHER,
};

typedef bool (*driver_probe_fn)(void);
typedef void (*driver_init_fn)(void);

struct driver {
    const char *name;
    enum driver_class class;
    driver_probe_fn probe;
    driver_init_fn init;
    const void *api;
};

bool driver_register(const struct driver *driver);
void driver_init_all(void);
const struct driver *driver_find(enum driver_class class);

#endif 
