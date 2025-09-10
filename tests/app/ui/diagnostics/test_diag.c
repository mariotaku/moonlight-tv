#include "e2e_base.h"
#include "app.h"
#include "ui/diagnostics/diag.fragment.h"

void test_basic() {
    lv_fragment_t *fragment = lv_fragment_create(&diag_fragment_class, NULL);
    lv_fragment_manager_push(app.ui.fm, fragment, &app.ui.container);
}

void initSettings(app_settings_t *settings) {
    app.ui.dpi = 240;
    settings->fullscreen = false;
    settings->window_state = (window_state_t) {
        .w = 1280,
        .h = 720,
    };
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_basic);
    return UNITY_END();
}