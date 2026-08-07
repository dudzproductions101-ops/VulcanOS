/*
 * string.c - VulcanOS libc string/memory function implementations
 *
 * Every function here is a from-scratch VulcanOS implementation.
 * None of this is copied from glibc, musl, newlib, or any other
 * existing C library -- the algorithms are the standard, well-known
 * ones (there is exactly one sane way to implement strlen), but the
 * actual code is written for this project.
 *
 * memmove specifically handles overlapping regions correctly (see
 * its own comment below) -- this is the one function in this file
 * where getting the algorithm right actually matters for
 * correctness, not just style, since memcpy's behavior is undefined
 * for overlapping regions by the C standard itself.
 */

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
    /* Standard strcmp contract: return value's SIGN indicates
     * ordering (negative if a<b, positive if a>b, zero if equal),
     * not merely "same or different" -- the unsigned-char cast
     * matters here because plain `char` signedness is
     * implementation-defined, and the standard specifies the
     * comparison must behave as if characters were unsigned. */
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
        /* body intentionally empty; the copy + NUL check happens in
         * the loop condition itself, which is the idiomatic (if
         * terse) standard way to write this -- kept exactly this
         * shape rather than "clarified" into a longer form, since
         * this specific idiom is universally recognized by any C
         * programmer and rewriting it would reduce clarity, not
         * improve it. */
    }
    return original_dest;
}

char *strncpy(char *dest, const char *src, usize n)
{
    usize i = 0;
    for (; i < n && src[i]; i++) {
        dest[i] = src[i];
    }
    /* Standard strncpy contract: pad the REMAINDER of dest with NUL
     * bytes up to n, even if src was shorter than n -- a real
     * standard-library quirk worth honoring exactly, since callers
     * (correctly or not) sometimes rely on this zero-padding
     * behavior. */
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
    /* Standard strchr contract: searching for the NUL terminator
     * itself is valid and should return a pointer to it, not NULL
     * -- this is why the check below exists after the loop rather
     * than being folded into the loop condition. */
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
        return (char *)s; /* pointer to the terminator itself */
    }
    return (char *)last;
}

char *strstr(const char *haystack, const char *needle)
{
    if (!*needle) {
        return (char *)haystack; /* standard contract: an empty needle
                                   * matches at the start of haystack */
    }

    for (; *haystack; haystack++) {
        const char *h = haystack;
        const char *n = needle;
        while (*h && *n && *h == *n) {
            h++;
            n++;
        }
        if (!*n) {
            return (char *)haystack; /* matched the entire needle */
        }
    }
    return NULL;
}

char *strdup(const char *s)
{
    /* Declared in stdlib.h, not string.h, in POSIX -- but
     * implemented here since it's a string operation at heart
     * (allocate + strcpy) and this keeps it next to strcpy's own
     * implementation. malloc itself lives in stdlib.c; this file
     * only calls it. */
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

/* Unlike memcpy, memmove must produce correct results even when
 * `dest` and `src` overlap. A naive forward byte-copy corrupts data
 * when dest is ahead of src within the overlapping region (each
 * byte gets overwritten by the copy before it's been read) -- the
 * standard fix, used here, is to copy backward (from the end) in
 * exactly that case, and forward otherwise. */
void *memmove(void *dest, const void *src, usize n)
{
    u8 *d = dest;
    const u8 *s = src;

    if (d == s || n == 0) {
        return dest;
    }

    if (d < s) {
        /* dest is before src: forward copy is always safe here,
         * since by the time we overwrite byte i, src[i] has already
         * been read (src is always ahead of the write position). */
        for (usize i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else {
        /* dest is after src (and they overlap, since d != s and we
         * already ruled out d < s): copy backward so each byte is
         * read from src before dest's write position reaches it. */
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
