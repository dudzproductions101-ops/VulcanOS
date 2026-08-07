












#ifndef VULCAN_PRINTK_H
#define VULCAN_PRINTK_H

#include "types.h"



enum log_level {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
};

void printk(const char *fmt, ...);
void printk_level(enum log_level level, const char *fmt, ...);

#endif 
