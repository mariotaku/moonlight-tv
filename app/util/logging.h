#pragma once

#include <stdio.h>
#include <time.h>

#ifdef APPLOG_TO_FILE
extern FILE *app_logfile;
#else
#define app_logfile stdout
#endif

#ifdef _GNU_SOURCE
#define applog_e(tag, fmt, ...) fprintf(app_logfile, "%10ld [" tag "][ERROR] " fmt "\n", time(NULL), ##__VA_ARGS__)
#define applog_w(tag, fmt, ...) fprintf(app_logfile, "%10ld [" tag "][WARN] " fmt "\n", time(NULL), ##__VA_ARGS__)
#define applog_i(tag, fmt, ...) fprintf(app_logfile, "%10ld [" tag "][INFO] " fmt "\n", time(NULL), ##__VA_ARGS__)
#ifdef DEBUG
#define applog_d(tag, fmt, ...) fprintf(app_logfile, "%10ld [" tag "][DEBUG] " fmt "\n", time(NULL), ##__VA_ARGS__)
#else
#define applog_d(tag, fmt, ...)
#endif
#else
#include <stdarg.h>
static void logprintf(const char *lvl, const char *tag, const char *fmt, ...)
{
    fprintf(app_logfile, "%10ld [%s][%s] ", tag, lvl, time(NULL));
    va_list args;
    va_start(args, fmt);
    vfprintf(app_logfile, fmt, args);
    va_end(args);
    fputs("\n", app_logfile);
}
#define applog_e(...) logprintf("ERROR", __VA_ARGS__)
#define applog_w(...) logprintf("WARN", __VA_ARGS__)
#define applog_i(...) logprintf("INFO", __VA_ARGS__)
#ifdef DEBUG
#define applog_d(...) logprintf("DEBUG", __VA_ARGS__)
#else
#define applog_d(...)
#endif
#endif