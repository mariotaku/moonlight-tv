#include "e2e_base.h"
#include "ui/settings/panes/pref_res.h"

void initSettings(app_settings_t *settings) {
    settings->fullscreen = false;
    settings->window_state = (window_state_t) {
        .w = 800,
        .h = 720,
    };
}

static int w, h;

void test_widescreen() {
    lv_obj_t *dropdown = pref_dropdown_res(app.ui.container, 3440, 1400, 3840, 2160, &w, &h);
    lv_obj_set_width(dropdown, LV_PCT(100));

    waitFor(300000);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_widescreen);
    return UNITY_END();
}