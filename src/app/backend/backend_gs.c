#include "app.h"
#include "logging.h"
#include "ui/fatal_error.h"
#include "client.h"
#include "errors.h"
#include "app_error.h"

GS_CLIENT app_gs_client_new(app_t *app) {
    if (SDL_ThreadID() == app->main_thread_id) {
        commons_log_fatal("APP", "%s MUST BE called from worker thread!", __FUNCTION__);
        abort();
    }
    SDL_assert_release(app->backend.gs_client_mutex != NULL);
    SDL_LockMutex(app->backend.gs_client_mutex);
    SDL_assert_release(app_configuration != NULL);
    GS_CLIENT client = gs_new(app_configuration->key_dir, app_configuration->debug_level);
    if (client == NULL && gs_get_error(NULL) == GS_BAD_CONF) {
        if (gs_conf_init(app_configuration->key_dir) != GS_OK) {
            const char *message = NULL;
            gs_get_error(&message);
            app_fatal_error("Failed to generate client info",
                            "Please turn off and unplug to completely restart the TV.\n\n"
                            "Details: %s", message);
            app_halt(app);
        } else {
            client = gs_new(app_configuration->key_dir, app_configuration->debug_level);
        }
    }
    if (client == NULL) {
        const char *message = NULL;
        gs_get_error(&message);
        app_fatal_error("Fatal error", "Failed to create GS_CLIENT: %s", message);
        app_halt(app);
    }
    SDL_UnlockMutex(app->backend.gs_client_mutex);
    return client;
}
