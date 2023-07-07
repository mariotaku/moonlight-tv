#include "unity.h"
#include "app.h"

app_t app;

void setUp(void) {
    char *argv[] = {"moonlight"};
    app_init(&app, 1, argv);
}

void tearDown(void) {
    app.running = false;
    app_deinit(&app);
}

void test_basic() {
    app_ui_open(&app.ui);
    app_run_loop(&app);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_basic);
    return UNITY_END();
}