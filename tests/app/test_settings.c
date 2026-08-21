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
    TEST_ASSERT_EQUAL_INT(TOUCHPAD_MODE_MOUSE, settings.touchpad_mode);
    TEST_ASSERT_EQUAL_INT(125, settings.touchpad_speed);
    TEST_ASSERT_FALSE(settings.touchpad_multitouch);
    TEST_ASSERT_FALSE(settings.touchpad_natural_scroll);
}

void testWriteINI() {
    settings.touchpad_mode = TOUCHPAD_MODE_MOUSE;
    settings.touchpad_speed = 175;
    settings.touchpad_multitouch = false;
    settings.touchpad_natural_scroll = false;

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

    TEST_ASSERT_EQUAL_INT(TOUCHPAD_MODE_MOUSE, loaded.touchpad_mode);
    TEST_ASSERT_EQUAL_INT(175, loaded.touchpad_speed);
    TEST_ASSERT_FALSE(loaded.touchpad_multitouch);
    TEST_ASSERT_FALSE(loaded.touchpad_natural_scroll);

    settings_clear(&loaded);
    remove("settings_write_tmp.ini");
}

void testControllerTouchpadDefaults() {
    TEST_ASSERT_EQUAL_INT(TOUCHPAD_MODE_NATIVE, settings.touchpad_mode);
    TEST_ASSERT_EQUAL_INT(TOUCHPAD_SPEED_DEFAULT, settings.touchpad_speed);
    TEST_ASSERT_TRUE(settings.touchpad_multitouch);
    TEST_ASSERT_TRUE(settings.touchpad_natural_scroll);
}

void testControllerTouchpadUnknownMode() {
    // Anything we don't recognise -- including "off", a third mode that early
    // revisions of this feature had and that never shipped -- has to land on the
    // default rather than on whatever happened to be in the struct.
    FILE *fp = fopen("settings_unknown_tmp.ini", "w");
    TEST_ASSERT_NOT_NULL(fp);
    fputs("[input]\ntouchpad_mode = off\n", fp);
    fclose(fp);

    app_settings_t unknown;
    settings_initialize(&unknown, settings.conf_dir);
    unknown.touchpad_mode = TOUCHPAD_MODE_MOUSE;
    char *unknown_ini_backup = unknown.ini_path;
    unknown.ini_path = "settings_unknown_tmp.ini";
    TEST_ASSERT_TRUE(settings_read(&unknown));
    unknown.ini_path = unknown_ini_backup;

    TEST_ASSERT_EQUAL_INT(TOUCHPAD_MODE_NATIVE, unknown.touchpad_mode);

    settings_clear(&unknown);
    remove("settings_unknown_tmp.ini");
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(testReadINI);
    RUN_TEST(testWriteINI);
    RUN_TEST(testControllerTouchpadDefaults);
    RUN_TEST(testControllerTouchpadUnknownMode);
    return UNITY_END();
}