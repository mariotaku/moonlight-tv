#include "app_error.h"
#include "app.h"

#include <SDL_thread.h>

#include "logging.h"

_Noreturn void app_halt(app_t *app) {
    if (SDL_ThreadID() == app->main_thread_id) {
        abort();
    } else {
        while (1);
    }
}

SDL_AssertState app_assertion_handler_abort(const SDL_AssertData *data, void *userdata) {
    (void) userdata;
    commons_log_fatal("Assertion", "at %s (%s:%d): '%s'", data->function, data->filename, data->linenum,
                      data->condition);
    return SDL_ASSERTION_ABORT;
}