#pragma once

#ifdef DEBUG

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

typedef void *(*malloc_func)(size_t);
typedef void *(*calloc_func)(size_t, size_t);
typedef void (*free_func)(void *);

static malloc_func _malloc_orig = &malloc;
static calloc_func _calloc_orig = &calloc;
static free_func _free_orig = &free;

static void *malloc_logged(size_t size, const char *file, int line)
{
    void *ret = _malloc_orig(size);
    printf("%s:%d malloc(%d) = %p\n", file, line, size, ret);
    return ret;
}

static void *calloc_logged(size_t n, size_t blksize, const char *file, int line)
{
    void *ret = _calloc_orig(n, blksize);
    printf("%s:%d calloc(%d) = %p\n", file, line, n * blksize, ret);
    return ret;
}

static void free_logged(void *p, const char *file, int line)
{
    _free_orig(p);
    printf("%s:%d free(%p)\n", file, line, p);
}

#define malloc(size) malloc_logged(size, __FILE__, __LINE__)
#define calloc(n, blksize) calloc_logged(n, blksize, __FILE__, __LINE__)
#define free(p) free_logged(p, __FILE__, __LINE__)

#endif