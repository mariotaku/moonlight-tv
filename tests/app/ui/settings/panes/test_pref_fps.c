#include "e2e_base.h"
#include "ui/settings/panes/pref_fps.h"

static int fps = 30;

void test_basic() {
    const static int options[] = {30, 60, 90, 120, 144, 240, 0};
    lv_obj_t *dropdown = pref_dropdown_fps(app.ui.container, options, 120, &fps);

    lv_obj_set_width(dropdown, LV_PCT(100));

    fakeTap(100, 48); // Simulate a tap on the dropdown

    waitFor(100);

    fakeTap(175, 489); // Click on "Custom" option

    waitFor(100);

    fakeKeyPress(SDLK_DOWN); // Focus on the slider

    fakeKeyPress(SDLK_RIGHT); // Increase the FPS value by 5

    fakeKeyPress(SDLK_DOWN); // Select dialog buttons

    fakeKeyPress(SDLK_RIGHT); // Move to "OK" button

    fakeKeyPress(SDLK_RETURN); // Press "OK" to confirm the selection

    UNITY_TEST_ASSERT_EQUAL_INT32(35, fps, __LINE__, "FPS value did not match expected value after interaction");

    app_request_exit();
}

void initSettings(app_settings_t *settings) {
    settings->fullscreen = false;
    settings->window_state = (window_state_t) {
        .w = 800,
        .h = 720,
    };
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_basic);
    return UNITY_END();
}