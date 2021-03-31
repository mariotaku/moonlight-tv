#pragma once

#ifdef DEBUG

#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef void *(*malloc_func)(size_t);
typedef void *(*calloc_func)(size_t, size_t);
typedef void (*free_func)(void *);

static malloc_func _malloc_orig = &malloc;
static calloc_func _calloc_orig = &calloc;
static free_func _free_orig = &free;

static FILE *logfile()
{
    static FILE *f = NULL;
    if (f)
        return f;
    char filename[64];
    snprintf(filename, sizeof(filename), "/tmp/memlog_%d.log", getpid());
    f = fopen(filename, "a");
    setvbuf(f, NULL, _IONBF, 0);
    return f;
}

static void *malloc_logged(size_t size, const char *file, const char *func, int line)
{
    void *ret = _malloc_orig(size);
    fprintf(logfile(), "%s:%d(%s) malloc %p %d\n", file, line, func, ret, size);
    return ret;
}

static void *calloc_logged(size_t n, size_t blksize, const char *file, const char *func, int line)
{
    void *ret = _calloc_orig(n, blksize);
    fprintf(logfile(), "%s:%d(%s) calloc %p %d\n", file, line, func, ret, n * blksize);
    return ret;
}

static void free_logged(void *p, const char *file, const char *func, int line)
{
    if (!p)
    {
        fprintf(stderr, "%s:%d(%s) free() on a null pointer\n", file, line, func);
        abort();
    }
    _free_orig(p);
    fprintf(logfile(), "%s:%d(%s) free %p\n", file, line, func, p);
}

#define __FILENAME__ (&(__FILE__)[SOURCE_DIR_LENGTH + 1])

#define malloc(size) malloc_logged(size, __FILENAME__, __FUNCTION__, __LINE__)
#define calloc(n, blksize) calloc_logged(n, blksize, __FILENAME__, __FUNCTION__, __LINE__)
#define free(p) free_logged((void *)p, __FILENAME__, __FUNCTION__, __LINE__)

#endif