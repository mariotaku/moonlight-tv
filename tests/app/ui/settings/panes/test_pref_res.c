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

void test_widescreen_native() {
    static int w = 3200, h = 1800;
    lv_obj_t *dropdown = pref_dropdown_res(app.ui.container, 3440, 1440, 3840, 2160, &w, &h);
    lv_obj_set_width(dropdown, LV_PCT(100));

    fakeTap(180, 60); // Tap on the dropdown to open it

    fakeTap(180, 480); // Tap on native resolution option

    UNITY_TEST_ASSERT_EQUAL_INT32(3440, w, __LINE__, "Width should be set to 3440");
    UNITY_TEST_ASSERT_EQUAL_INT32(1440, h, __LINE__, "Height should be set to 1440");
}

void test_widescreen_custom() {
    static int w = 3440, h = 1440;
    lv_obj_t *dropdown = pref_dropdown_res(app.ui.container, 3440, 1440, 3840, 2160, &w, &h);
    lv_obj_set_width(dropdown, LV_PCT(100));

    fakeTap(180, 60); // Tap on the dropdown to open it

    fakeTap(180, 560); // Tap on custom resolution option

    fakeTap(250, 380); // Tap on the first input field to set width

    fakeInput("3440");

    fakeTap(460, 380); // Tap on the first input field to set width

    fakeInput("720");

    fakeTap(490, 500); // Tap on the OK button

    UNITY_TEST_ASSERT_EQUAL_INT32(3440, w, __LINE__, "Width should be set to 3440");
    UNITY_TEST_ASSERT_EQUAL_INT32(720, h, __LINE__, "Height should be set to 1440");
}

void test_custom_invalid() {
    static int w = 3840, h = 2160;
    lv_obj_t *dropdown = pref_dropdown_res(app.ui.container, 3840, 2160, 3840, 2160, &w, &h);
    lv_obj_set_width(dropdown, LV_PCT(100));

    fakeTap(180, 60); // Tap on the dropdown to open it

    fakeTap(180, 560); // Tap on custom resolution option

    fakeTap(250, 380); // Tap on the first input field to set width

    fakeInput("8888");

    fakeTap(460, 380); // Tap on the first input field to set width

    fakeInput("8888");

    fakeTap(320, 500); // Tap on the cancel button

    UNITY_TEST_ASSERT_EQUAL_INT32(3840, w, __LINE__, "Width should be set to 3840");
    UNITY_TEST_ASSERT_EQUAL_INT32(2160, h, __LINE__, "Height should be set to 2160");
}

void test_custom_key_input() {
    static int w = 3840, h = 2160;
    lv_obj_t *dropdown = pref_dropdown_res(app.ui.container, 3840, 2160, 3840, 2160, &w, &h);
    lv_obj_set_width(dropdown, LV_PCT(100));

    fakeKeyPress(SDLK_RETURN); // Simulate pressing Enter to open the dropdown

    fakeKeyPress(SDLK_DOWN); // Navigate to the custom resolution option

    fakeKeyPress(SDLK_RETURN); // Select the custom resolution option

    fakeInput("960"); // Input width

    fakeKeyPress(SDLK_RIGHT);
    fakeKeyPress(SDLK_RIGHT);

    fakeInput("540"); // Input height

    fakeKeyPress(SDLK_DOWN);
    fakeKeyPress(SDLK_RIGHT); // Navigate to the OK button

    fakeKeyPress(SDLK_RETURN); // Press Enter to confirm

    UNITY_TEST_ASSERT_EQUAL_INT32(960, w, __LINE__, "Width should be set to 960");
    UNITY_TEST_ASSERT_EQUAL_INT32(540, h, __LINE__, "Height should be set to 540");
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_select_720p);
    RUN_TEST(test_widescreen_native);
    RUN_TEST(test_widescreen_custom);
    RUN_TEST(test_custom_invalid);
    RUN_TEST(test_custom_key_input);
    return UNITY_END();
}