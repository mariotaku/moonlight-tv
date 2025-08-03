#include <stdlib.h>
#include <stdio.h>

#include "e2e_base.h"
#include "uuidstr.h"

static int argc = 1;
static char *argv[] = {"moonlight"};

app_t app;

static int settingsLoader(app_settings_t *settings) {
    char *path = malloc(128);
    uuidstr_t uuid;
    uuidstr_random(&uuid);
    snprintf(path, 128, "/tmp/moonlight-test-%s", (char *) &uuid);
    settings_initialize(settings, path);
    initSettings(settings);
    return 0;
}

void setUp(void) {
    app_init(&app, settingsLoader, argc, argv);
    app_ui_open(&app.ui, false, NULL);
}

void tearDown(void) {
    while (app.running) {
        app_run_loop(&app);
    }
    app.running = false;
    app_deinit(&app);
}
