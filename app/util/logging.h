#pragma once

#ifdef APPLOG_HOST
void app_logvprintf(const char *lvl, const char *tag, const char *fmt, va_list args);
void app_logprintf(const char *lvl, const char *tag, const char *fmt, ...);
#else
#include <stdarg.h>

typedef void (*logvprintf_fn)(const char *, const char *, const char *, va_list);

extern logvprintf_fn module_logvprintf;
static void app_logprintf(const char *lvl, const char *tag, const char *fmt, ...)
{
    if (!module_logvprintf)
        return;
    va_list args;
    va_start(args, fmt);
    module_logvprintf(lvl, tag, fmt, args);
    va_end(args);
}
#endif

#define applog_e(...) app_logprintf("ERROR", __VA_ARGS__)
#define applog_w(...) app_logprintf("WARN", __VA_ARGS__)
#define applog_i(...) app_logprintf("INFO", __VA_ARGS__)
#ifdef DEBUG
#define applog_d(...) app_logprintf("DEBUG", __VA_ARGS__)
#else
#define applog_d(...)
#endif