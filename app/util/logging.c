#include "app.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#ifndef APPLOG_FILE
#define APPLOG_FILE stdout
#endif

void app_logvprintf(const char *lvl, const char *tag, const char *fmt, va_list args)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    fprintf(APPLOG_FILE, "%06ld.%03d [%s][%s] ", ts.tv_sec, ts.tv_nsec / 1000000UL, tag, lvl);
    vfprintf(APPLOG_FILE, fmt, args);
    fputs("\n", APPLOG_FILE);
}

void app_logprintf(const char *lvl, const char *tag, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    app_logvprintf(lvl, tag, fmt, args);
    va_end(args);
}
