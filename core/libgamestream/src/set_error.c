#include "set_error.h"
#include "errors.h"

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

static int gs_errno = GS_OK;
static char gs_errmsg[1024];

int gs_get_error(const char **message) {
    if (message != NULL) {
        *message = gs_errmsg;
    }
    return gs_errno;
}

int gs_set_error(int error, const char *fmt, ...) {
    gs_errno = error;
    if (fmt != NULL) {
        va_list arg;
        va_start(arg, fmt);
        vsnprintf(gs_errmsg, 1024, fmt, arg);
        va_end(arg);
    } else {
        gs_errmsg[0] = '\0';
    }
    return error;
}