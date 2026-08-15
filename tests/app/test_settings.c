#include <stdio.h>
#include <stdlib.h>
#include "unity.h"
#include "app_settings.h"
#include "uuidstr.h"

#ifndef FIXTURES_PATH_PREFIX
#define FIXTURES_PATH_PREFIX "./"
#endif

app_settings_t settings;

void setUp() {
    char *dir = malloc(128);
    uuidstr_t uuid;
    uuidstr_random(&uuid);
    snprintf(dir, 128, "/tmp/moonlight-%s", (char *) &uuid);
    settings_initialize(&settings, dir);
}

void tearDown() {
    settings_clear(&settings);
    free(settings.conf_dir);
}

void testReadINI() {
    char *ini_backup = settings.ini_path;
    settings.ini_path = FIXTURES_PATH_PREFIX "settings_read.ini";
    TEST_ASSERT_TRUE(settings_read(&settings));
    settings.ini_path = ini_backup;
    TEST_ASSERT_EQUAL_INT(CONTROLLER_TOUCHPAD_MODE_MOUSE, settings.controller_touchpad_mode);
    TEST_ASSERT_EQUAL_INT(125, settings.controller_touchpad_sensitivity);
    TEST_ASSERT_EQUAL_INT(CONTROLLER_TOUCHPAD_PRESS_RIGHT, settings.controller_touchpad_press);
    TEST_ASSERT_EQUAL_INT(CONTROLLER_TOUCHPAD_PRESS_MIDDLE, settings.controller_touchpad_secondary_click);
    TEST_ASSERT_FALSE(settings.controller_touchpad_tap_to_click);
    TEST_ASSERT_TRUE(settings.controller_touchpad_two_finger_scroll);
    TEST_ASSERT_FALSE(settings.controller_touchpad_invert_two_finger_scroll);
}

void testWriteINI() {
    settings.controller_touchpad_mode = CONTROLLER_TOUCHPAD_MODE_NATIVE;
    settings.controller_touchpad_sensitivity = 175;
    settings.controller_touchpad_press = CONTROLLER_TOUCHPAD_PRESS_MIDDLE;
    settings.controller_touchpad_secondary_click = CONTROLLER_TOUCHPAD_PRESS_LEFT;
    settings.controller_touchpad_tap_to_click = false;
    settings.controller_touchpad_two_finger_scroll = true;
    settings.controller_touchpad_invert_two_finger_scroll = false;

    char *ini_backup = settings.ini_path;
    settings.ini_path = "settings_write_tmp.ini";
    TEST_ASSERT_TRUE(settings_save(&settings));
    settings.ini_path = ini_backup;

    app_settings_t loaded;
    settings_initialize(&loaded, settings.conf_dir);
    char *loaded_ini_backup = loaded.ini_path;
    loaded.ini_path = "settings_write_tmp.ini";
    TEST_ASSERT_TRUE(settings_read(&loaded));
    loaded.ini_path = loaded_ini_backup;

    TEST_ASSERT_EQUAL_INT(CONTROLLER_TOUCHPAD_MODE_NATIVE, loaded.controller_touchpad_mode);
    TEST_ASSERT_EQUAL_INT(175, loaded.controller_touchpad_sensitivity);
    TEST_ASSERT_EQUAL_INT(CONTROLLER_TOUCHPAD_PRESS_MIDDLE, loaded.controller_touchpad_press);
    TEST_ASSERT_EQUAL_INT(CONTROLLER_TOUCHPAD_PRESS_LEFT, loaded.controller_touchpad_secondary_click);
    TEST_ASSERT_FALSE(loaded.controller_touchpad_tap_to_click);
    TEST_ASSERT_TRUE(loaded.controller_touchpad_two_finger_scroll);
    TEST_ASSERT_FALSE(loaded.controller_touchpad_invert_two_finger_scroll);

    settings_clear(&loaded);
    remove("settings_write_tmp.ini");
}

void testControllerTouchpadDefaults() {
    TEST_ASSERT_EQUAL_INT(CONTROLLER_TOUCHPAD_MODE_MOUSE, settings.controller_touchpad_mode);
    TEST_ASSERT_EQUAL_INT(CONTROLLER_TOUCHPAD_SENSITIVITY_DEFAULT, settings.controller_touchpad_sensitivity);
    TEST_ASSERT_EQUAL_INT(CONTROLLER_TOUCHPAD_PRESS_LEFT, settings.controller_touchpad_press);
    TEST_ASSERT_EQUAL_INT(CONTROLLER_TOUCHPAD_PRESS_RIGHT, settings.controller_touchpad_secondary_click);
    TEST_ASSERT_TRUE(settings.controller_touchpad_tap_to_click);
    TEST_ASSERT_TRUE(settings.controller_touchpad_two_finger_scroll);
    TEST_ASSERT_TRUE(settings.controller_touchpad_invert_two_finger_scroll);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(testReadINI);
    RUN_TEST(testWriteINI);
    RUN_TEST(testControllerTouchpadDefaults);
    return UNITY_END();
}