#include "app.h"
#include "logging.h"

int main(int argc, char *argv[]) {
    app_t app_;
    int ret = app_init(&app_, argc, argv);
    if (ret != 0) {
        return ret;
    }

    app_handle_launch(argc, argv);

    if (strlen(app_configuration->default_host_uuid) > 0) {
        commons_log_info("APP", "Will launch with host %s and app %d", app_configuration->default_host_uuid,
                         app_configuration->default_app_id);
    }

    app_ui_open(&app_.ui);

    while (app_.running) {
        app_run_loop(&app_);
    }

    app_deinit(&app_);
    return ret;
}
