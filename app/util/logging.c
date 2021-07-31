#include "app.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifndef APPLOG_FILE
#define APPLOG_FILE stdout
#endif

static struct timespec app_start_time;

/**
 * @fn timespec_diff(struct timespec *, struct timespec *, struct timespec *)
 * @brief Compute the diff of two timespecs, that is a - b = result.
 * @param a the minuend
 * @param b the subtrahend
 * @param result a - b
 */
static inline void timespec_diff(struct timespec *a, struct timespec *b, struct timespec *result)
{
    result->tv_sec = a->tv_sec - b->tv_sec;
    result->tv_nsec = a->tv_nsec - b->tv_nsec;
    if (result->tv_nsec < 0)
    {
        --result->tv_sec;
        result->tv_nsec += 1000000000L;
    }
}

void app_logvprintf(const char *lvl, const char *tag, const char *fmt, va_list args)
{
    struct timespec ts, delta;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    timespec_diff(&ts, &app_start_time, &delta);
    fprintf(APPLOG_FILE, "%06ld.%03ld [%s][%s] ", delta.tv_sec, delta.tv_nsec / 1000000L, tag, lvl);
    size_t len = strlen(fmt);
    vfprintf(APPLOG_FILE, fmt, args);
    if (len && fmt[len - 1] != '\n')
        fputs("\n", APPLOG_FILE);
}

void app_logprintf(const char *lvl, const char *tag, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    app_logvprintf(lvl, tag, fmt, args);
    va_end(args);
}

void app_loginit()
{
    clock_gettime(CLOCK_MONOTONIC, &app_start_time);
}