#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdarg.h>

#ifndef MODULE_LOGVPRINTF
#define MODULE_LOGVPRINTF module_logvprintf
#endif
#ifndef MODULE_EMBEDDED
#define MODULE_EMBEDDED 0
#endif

typedef enum applog_level_t {
    APPLOG_VERBOSE = 0,
    APPLOG_DEBUG,
    APPLOG_INFO,
    APPLOG_WARN,
    APPLOG_ERROR,
    APPLOG_FATAL,
    APPLOG_MAX = 0x7FFFFFFF,
} applog_level_t;

typedef void (*logvprintf_fn)(int, const char *, const char *, va_list);

#ifdef MODULE_EMBEDDED

void app_logprintf(applog_level_t lvl, const char *tag, const char *fmt, ...);

void app_logvprintf(applog_level_t lvl, const char *tag, const char *fmt, va_list args);

#else

extern logvprintf_fn MODULE_LOGVPRINTF;

#define app_logvprintf(lvl, tag, fmt, args) MODULE_LOGVPRINTF(lvl, tag, fmt, args)

static void app_logprintf(applog_level_t lvl, const char *tag, const char *fmt, ...) {
    if (!MODULE_LOGVPRINTF)
        return;
    va_list args;
    va_start(args, fmt);
    MODULE_LOGVPRINTF(lvl, tag, fmt, args);
    va_end(args);
}
#endif

#define applog(level, ...) app_logprintf(level, __VA_ARGS__)
#define applog_f(...) app_logprintf(APPLOG_FATAL, __VA_ARGS__)
#define applog_e(...) app_logprintf(APPLOG_ERROR, __VA_ARGS__)
#define applog_w(...) app_logprintf(APPLOG_WARN, __VA_ARGS__)
#define applog_i(...) app_logprintf(APPLOG_INFO, __VA_ARGS__)
#ifdef DEBUG
#define applog_d(...) app_logprintf(APPLOG_DEBUG, __VA_ARGS__)
#define applog_v(...) app_logprintf(APPLOG_VERBOSE, __VA_ARGS__)
#else
#define applog_d(...)
#define applog_v(...)
#endif

#ifdef __cplusplus
}
#endif