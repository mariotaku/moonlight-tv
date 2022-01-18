#include <string.h>

#include "module/logging.h"

__attribute__((unused)) int _CGL_LOG_DEBUG(const char *tag, const char *fmt, ...) {
    char app_tag[64];
    strcpy(app_tag, "CGL.");
    strcat(app_tag, tag);
    va_list arglist;
    va_start(arglist, fmt);
    app_logvprintf(APPLOG_DEBUG, app_tag, fmt, arglist);
    va_end(arglist);
    return 0;
}

__attribute__((unused)) int _CGL_LOG_INFO(const char *tag, const char *fmt, ...) {
    char app_tag[64];
    strcpy(app_tag, "CGL.");
    strcat(app_tag, tag);
    va_list arglist;
    va_start(arglist, fmt);
    app_logvprintf(APPLOG_INFO, app_tag, fmt, arglist);
    va_end(arglist);
    return 0;
}

__attribute__((unused)) int _CGL_LOG_WARNING(const char *tag, const char *fmt, ...) {
    char app_tag[64];
    strcpy(app_tag, "CGL.");
    strcat(app_tag, tag);
    va_list arglist;
    va_start(arglist, fmt);
    app_logvprintf(APPLOG_WARN, app_tag, fmt, arglist);
    va_end(arglist);
    return 0;
}

__attribute__((unused)) int _CGL_LOG_ERROR(const char *tag, const char *fmt, ...) {
    char app_tag[64];
    strcpy(app_tag, "CGL.");
    strcat(app_tag, tag);
    va_list arglist;
    va_start(arglist, fmt);
    app_logvprintf(APPLOG_ERROR, app_tag, fmt, arglist);
    va_end(arglist);
    return 0;
}