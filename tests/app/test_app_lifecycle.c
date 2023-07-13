#include "unity.h"
#include "app.h"
#include "uuidstr.h"
#include "app_launch.h"

static int argc = 1;
static char *argv[] = {"moonlight"};
app_t app;

Uint32 quitTimer(Uint32 interval, void *context) {
    (void) interval;
    (void) context;
    SDL_Event event = {
            .type = SDL_QUIT,
    };
    SDL_PushEvent(&event);
    return 0;
}

int initSettings(app_settings_t *settings) {
    char *path = malloc(128);
    uuidstr_t uuid;
    uuidstr_random(&uuid);
    snprintf(path, 128, "/tmp/moonlight-test-%s", (char *) &uuid);
    settings_initialize(settings, path);
    return 0;
}

void setUp(void) {
    app_init(&app, initSettings, argc, argv);
}

void tearDown(void) {
    app.running = false;
    app_deinit(&app);
}

void test_basic() {
    app_ui_open(&app.ui, NULL);

    SDL_AddTimer(1000, quitTimer, &app);
    while (app.running) {
        app_run_loop(&app);
    }
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_basic);
    return UNITY_END();
}