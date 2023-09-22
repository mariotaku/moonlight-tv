#include "app.h"
#include "app_launch.h"
#include "util/path.h"
#include "logging.h"

static int settings_load(app_settings_t *settings);

int main(int argc, char *argv[]) {
#if TARGET_WEBOS
    if (getenv("EGL_PLATFORM") == NULL) {
        setenv("EGL_PLATFORM", "wayland", 0);
    }
    if (getenv("XDG_RUNTIME_DIR") == NULL) {
        setenv("XDG_RUNTIME_DIR", "/tmp/xdg", 0);
    }
#endif
    app_t app;
    int ret = app_init(&app, settings_load, argc, argv);
    if (ret != 0) {
        return ret;
    }

    app_launch_params_t *params = app_handle_launch(&app, argc, argv);

    app_ui_open(&app.ui, params);

    while (app.running) {
        app_run_loop(&app);
    }

    app_launch_param_free(params);

    app_deinit(&app);
    return ret;
}


static int settings_load(app_settings_t *settings) {
    bool persistent = true;
    settings_initialize(settings, path_pref(&persistent));
    settings->conf_persistent = persistent;
    if (!settings_read(settings)) {
        commons_log_warn("Settings", "Failed to read settings %s", settings->ini_path);
    }
    return 0;
}