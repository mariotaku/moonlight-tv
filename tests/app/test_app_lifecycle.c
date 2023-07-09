#include "unity.h"
#include "app.h"

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

void setUp(void) {
    app_init(&app, argc, argv);
}

void tearDown(void) {
    app.running = false;
    app_deinit(&app);
}

void test_basic() {
    app_handle_launch(argc, argv);

    app_ui_open(&app.ui);

    SDL_AddTimer(10000, quitTimer, NULL);

    while (app.running) {
        app_run_loop(&app);
    }
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_basic);
    return UNITY_END();
}