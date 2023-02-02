#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdarg.h>

typedef enum applog_level_t {
    APPLOG_VERBOSE = 0,
    APPLOG_DEBUG,
    APPLOG_INFO,
    APPLOG_WARN,
    APPLOG_ERROR,
    APPLOG_FATAL,
    APPLOG_MAX = 0x7FFFFFFF,
} applog_level_t;

static const char *applog_level_str[] = {
        "VERBOSE",
        "DEBUG",
        "INFO",
        "WARN",
        "ERROR",
        "FATAL",
};

void app_logvprintf(applog_level_t lvl, const char *tag, const char *fmt, va_list args);

void app_logprintf(applog_level_t lvl, const char *tag, const char *fmt, ...) __attribute__ ((format (printf, 3, 4)));

void app_loginit();

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