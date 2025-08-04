#include "e2e_base.h"
#include "ui/settings/panes/pref_res.h"

void initSettings(app_settings_t *settings) {
    settings->fullscreen = false;
    settings->window_state = (window_state_t) {
        .w = 800,
        .h = 720,
    };
}


void test_select_720p() {
    static int w = 1920, h = 1080;
    lv_obj_t *dropdown = pref_dropdown_res(app.ui.container, 1920, 1080, 1920, 1080, &w, &h);
    lv_obj_set_width(dropdown, LV_PCT(100));

    fakeTap(180, 60); // Tap on the dropdown to open it

    fakeTap(100, 170); // Tap on the 720p option

    UNITY_TEST_ASSERT_EQUAL_INT32(1280, w, __LINE__, "Width should be set to 3440");
    UNITY_TEST_ASSERT_EQUAL_INT32(720, h, __LINE__, "Height should be set to 1440");
}

void test_widescreen() {
    static int w = 1920, h = 1080;
    lv_obj_t *dropdown = pref_dropdown_res(app.ui.container, 3440, 1400, 3840, 2160, &w, &h);
    lv_obj_set_width(dropdown, LV_PCT(100));

    fakeTap(180, 60); // Tap on the dropdown to open it

    fakeTap(180, 560); // Tap on custom resolution option

    fakeTap(250, 380); // Tap on the first input field to set width

    fakeInput("3440");

    fakeTap(460, 380); // Tap on the first input field to set width

    fakeInput("1440");

    fakeTap(490, 500); // Tap on the OK button

    UNITY_TEST_ASSERT_EQUAL_INT32(3440, w, __LINE__, "Width should be set to 3440");
    UNITY_TEST_ASSERT_EQUAL_INT32(1440, h, __LINE__, "Height should be set to 1440");
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_select_720p);
    RUN_TEST(test_widescreen);
    return UNITY_END();
}