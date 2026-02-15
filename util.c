/**
 * util.c - Small, reusable utility functions.
 */

#include "vpsm.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

/* Print a formatted error message and exit. */
void die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "vpsm: error: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

/* Strip leading and trailing whitespace in-place. */
void trim(char *s)
{
    if (!s) return;

    /* Leading whitespace */
    size_t start = 0;
    while (s[start] && isspace((unsigned char)s[start]))
        start++;

    if (start > 0)
        memmove(s, s + start, strlen(s) - start + 1);

    /* Trailing whitespace */
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1]))
        s[--len] = '\0';
}

/* Case-sensitive string equality helper. */
int str_eq(const char *a, const char *b)
{
    return strcmp(a, b) == 0;
}

/*
 * Null-safe strncpy that always null-terminates.
 * dst receives at most (n-1) characters from src.
 */
void safe_strncpy(char *dst, const char *src, size_t n)
{
    if (!dst || n == 0) return;
    strncpy(dst, src ? src : "", n - 1);
    dst[n - 1] = '\0';
}
