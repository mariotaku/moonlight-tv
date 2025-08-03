#include "e2e_base.h"
#include "ui/settings/panes/pref_fps.h"

void initSettings(app_settings_t *settings) {
    settings->fullscreen = false;
    settings->window_state = (window_state_t) {
        .w = 1920,
        .h = 1080,
    };
}

static int fps = 30;

void test_basic() {
    const static int options[] = {30, 60, 90, 120, 144, 240, 0};
    lv_obj_t *dropdown = pref_dropdown_fps(app.ui.container, options, 120, &fps);

    lv_obj_set_width(dropdown, LV_DPX(300));
    lv_obj_center(dropdown);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_basic);
    return UNITY_END();
}