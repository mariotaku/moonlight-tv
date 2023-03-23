#include "logging.h"
#include "ss4s/logging.h"

#include <stdarg.h>

void app_logprintf(applog_level_t lvl, const char *tag, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    app_logvprintf(lvl, tag, fmt, args);
    va_end(args);
}

void applog_ss4s(SS4S_LogLevel level, const char *tag, const char *fmt, ...) {
    applog_level_t app_level;
    switch (level) {
        case SS4S_LogLevelFatal:
            app_level = APPLOG_FATAL;
            break;
        case SS4S_LogLevelError:
            app_level = APPLOG_ERROR;
            break;
        case SS4S_LogLevelWarn:
            app_level = APPLOG_WARN;
            break;
        case SS4S_LogLevelInfo:
            app_level = APPLOG_INFO;
            break;
        case SS4S_LogLevelDebug:
            app_level = APPLOG_DEBUG;
            break;
        case SS4S_LogLevelVerbose:
            app_level = APPLOG_VERBOSE;
            break;
    }
    va_list args;
    va_start(args, fmt);
    app_logvprintf(app_level, tag, fmt, args);
    va_end(args);
}