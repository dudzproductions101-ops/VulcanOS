








#ifndef VULCAN_LIBC_STDLIB_H
#define VULCAN_LIBC_STDLIB_H

#include "vulcan_types.h"

void *malloc(usize size);
void free(void *ptr);

int atoi(const char *s);
long atol(const char *s);






__attribute__((noreturn)) void exit(int status);













void yield(void);





void sleep(int seconds);

#endif 
