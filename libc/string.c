















#include "string.h"

usize strlen(const char *s)
{
    usize len = 0;
    while (s[len]) {
        len++;
    }
    return len;
}

int strcmp(const char *a, const char *b)
{
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    





    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

int strncmp(const char *a, const char *b, usize n)
{
    for (usize i = 0; i < n; i++) {
        if (a[i] != b[i] || a[i] == '\0') {
            return (int)(unsigned char)a[i] - (int)(unsigned char)b[i];
        }
    }
    return 0;
}

char *strcpy(char *dest, const char *src)
{
    char *original_dest = dest;
    while ((*dest++ = *src++)) {
        






    }
    return original_dest;
}

char *strncpy(char *dest, const char *src, usize n)
{
    usize i = 0;
    for (; i < n && src[i]; i++) {
        dest[i] = src[i];
    }
    




    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}

char *strcat(char *dest, const char *src)
{
    char *original_dest = dest;
    while (*dest) {
        dest++;
    }
    while ((*dest++ = *src++)) {
    }
    return original_dest;
}

char *strchr(const char *s, int c)
{
    while (*s) {
        if (*s == (char)c) {
            return (char *)s;
        }
        s++;
    }
    



    if ((char)c == '\0') {
        return (char *)s;
    }
    return NULL;
}

char *strrchr(const char *s, int c)
{
    const char *last = NULL;
    while (*s) {
        if (*s == (char)c) {
            last = s;
        }
        s++;
    }
    if ((char)c == '\0') {
        return (char *)s; 
    }
    return (char *)last;
}

char *strstr(const char *haystack, const char *needle)
{
    if (!*needle) {
        return (char *)haystack; 

    }

    for (; *haystack; haystack++) {
        const char *h = haystack;
        const char *n = needle;
        while (*h && *n && *h == *n) {
            h++;
            n++;
        }
        if (!*n) {
            return (char *)haystack; 
        }
    }
    return NULL;
}

char *strdup(const char *s)
{
    




    extern void *malloc(usize size);

    usize len = strlen(s) + 1;
    char *copy = malloc(len);
    if (!copy) {
        return NULL;
    }
    for (usize i = 0; i < len; i++) {
        copy[i] = s[i];
    }
    return copy;
}

void *memcpy(void *dest, const void *src, usize n)
{
    u8 *d = dest;
    const u8 *s = src;
    for (usize i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}







void *memmove(void *dest, const void *src, usize n)
{
    u8 *d = dest;
    const u8 *s = src;

    if (d == s || n == 0) {
        return dest;
    }

    if (d < s) {
        


        for (usize i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else {
        


        for (usize i = n; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    }

    return dest;
}

void *memset(void *dest, int value, usize n)
{
    u8 *d = dest;
    u8 v = (u8)value;
    for (usize i = 0; i < n; i++) {
        d[i] = v;
    }
    return dest;
}

int memcmp(const void *a, const void *b, usize n)
{
    const u8 *pa = a;
    const u8 *pb = b;
    for (usize i = 0; i < n; i++) {
        if (pa[i] != pb[i]) {
            return (int)pa[i] - (int)pb[i];
        }
    }
    return 0;
}
