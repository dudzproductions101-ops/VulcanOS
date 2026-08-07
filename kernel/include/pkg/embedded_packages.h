




































#ifndef VULCAN_PKG_EMBEDDED_PACKAGES_H
#define VULCAN_PKG_EMBEDDED_PACKAGES_H

#include "types.h"

struct embedded_package {
    const char *install_name;   




    const u8 *data;
    usize size;
};






const struct embedded_package *embedded_package_find(const char *install_name);





u32 embedded_package_count(void);
const struct embedded_package *embedded_package_at(u32 index);

#endif 
