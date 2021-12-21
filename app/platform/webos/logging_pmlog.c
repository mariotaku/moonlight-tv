#include "util/logging.h"

#include <stdarg.h>
#include <stdio.h>

#include <PmLogLib.h>
#include <SDL.h>

static SDL_mutex *mutex;

void app_logvprintf(applog_level_t lvl, const char *tag, const char *fmt, va_list args) {
    PmLogContext context;
    PmLogGetContext("moonlight", &context);
    char msg[1024];
    vsnprintf(msg, sizeof(msg), fmt, args);
    Uint32 ticks = SDL_GetTicks();
    SDL_LockMutex(mutex);
    size_t msglen = strnlen(msg, 1024);
    if (msg[msglen - 1] == '\n') {
        msglen -= 1;
    }
    printf("%06ld.%03ld [%s][%s] %.*s\n", ticks / 1000L, ticks % 1000L, applog_level_str[lvl], tag, msglen, msg);
    SDL_UnlockMutex(mutex);
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
            PmLogDebug(context, "[%s] %s", tag, msg);
            break;
    }
}

void app_loginit() {
    mutex = SDL_CreateMutex();
}