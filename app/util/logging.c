#include "logging.h"

#include <stdarg.h>

void app_logprintf(applog_level_t lvl, const char *tag, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    app_logvprintf(lvl, tag, fmt, args);
    va_end(args);
}