#include "app.h"
#include "util/logging.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static struct timespec app_start_time;

/**
 * @fn timespec_diff(struct timespec *, struct timespec *, struct timespec *)
 * @brief Compute the diff of two timespecs, that is a - b = result.
 * @param a the minuend
 * @param b the subtrahend
 * @param result a - b
 */
static inline void timespec_diff(struct timespec *a, struct timespec *b, struct timespec *result) {
    result->tv_sec = a->tv_sec - b->tv_sec;
    result->tv_nsec = a->tv_nsec - b->tv_nsec;
    if (result->tv_nsec < 0) {
        --result->tv_sec;
        result->tv_nsec += 1000000000L;
    }
}

void app_logvprintf(applog_level_t lvl, const char *tag, const char *fmt, va_list args) {
    struct timespec ts, delta;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    timespec_diff(&ts, &app_start_time, &delta);
    printf("%06ld.%03ld [%s][%s] ", delta.tv_sec, delta.tv_nsec / 1000000L, tag, applog_level_str[lvl]);
    size_t len = strlen(fmt);
    vprintf(fmt, args);
    if (len && fmt[len - 1] != '\n')
        printf("\n");
}

void app_loginit() {
    clock_gettime(CLOCK_MONOTONIC, &app_start_time);
}