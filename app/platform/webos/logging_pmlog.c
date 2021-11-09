#include "util/logging.h"

#include <stdarg.h>
#include <stdio.h>

#include <PmLogLib.h>

void app_logvprintf(applog_level_t lvl, const char *tag, const char *fmt, va_list args) {
    PmLogContext context;
    PmLogGetContext("moonlight", &context);
    char msg[1024];
    vsnprintf(msg, sizeof(msg), fmt, args);
    printf("[%s][%s] %s\n", applog_level_str[lvl], tag, msg);
    switch (lvl) {
        case APPLOG_INFO:
            PmLogInfo(context, tag, 0, "%s", msg);
            break;
        case APPLOG_WARN:
            PmLogWarning(context, tag, 0, "%s", msg);
            break;
        case APPLOG_ERROR:
            PmLogError(context, tag, 0, "%s", msg);
            break;
        case APPLOG_FATAL:
            PmLogCritical(context, tag, 0, "%s", msg);
            break;
        case APPLOG_VERBOSE:
        case APPLOG_DEBUG:
        default:
            PmLogDebug(context, tag, 0, "%s", msg);
            break;
    }
}

void app_loginit() {

}