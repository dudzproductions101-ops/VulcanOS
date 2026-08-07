#ifndef VULCAN_LIBC_STRING_H
#define VULCAN_LIBC_STRING_H

#include "vulcan_types.h"

usize strlen(const char *s);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, usize n);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, usize n);
char *strcat(char *dest, const char *src);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
char *strstr(const char *haystack, const char *needle);
char *strdup(const char *s);

void *memcpy(void *dest, const void *src, usize n);
void *memmove(void *dest, const void *src, usize n);
void *memset(void *dest, int value, usize n);
int memcmp(const void *a, const void *b, usize n);

#endif 
