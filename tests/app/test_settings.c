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
}

void testWriteINI() {
    char *ini_backup = settings.ini_path;
    settings.ini_path = "settings_write_tmp.ini";
    TEST_ASSERT_TRUE(settings_save(&settings));
    settings.ini_path = ini_backup;
}

void testUnlimitedBitrateRoundTrip() {
    char *ini_backup = settings.ini_path;
    settings.ini_path = "settings_unlimited_tmp.ini";
    settings.bitrate_unlimited = true;
    settings.stream.bitrate = BITRATE_MAX;
    TEST_ASSERT_TRUE(settings_save(&settings));

    settings.bitrate_unlimited = false;
    settings.stream.bitrate = 0;
    TEST_ASSERT_TRUE(settings_read(&settings));
    TEST_ASSERT_TRUE(settings.bitrate_unlimited);
    // The flag carries the mode, so the numeric bitrate survives untouched.
    TEST_ASSERT_EQUAL_INT(BITRATE_MAX, settings.stream.bitrate);
    settings.ini_path = ini_backup;
}

void testEffectiveBitrate() {
    settings.bitrate_unlimited = false;
    settings.stream.bitrate = 25000;
    TEST_ASSERT_EQUAL_INT(25000, settings_effective_bitrate(&settings, NULL));

    settings.bitrate_unlimited = true;
    TEST_ASSERT_EQUAL_INT(BITRATE_UNLIMITED_KBPS, settings_effective_bitrate(&settings, NULL));

    // A negative bitrate means "automatic" and must never leak through as a negative.
    settings.bitrate_unlimited = false;
    settings.stream.bitrate = -1;
    TEST_ASSERT_EQUAL_INT(settings_optimal_bitrate(NULL, settings.stream.width, settings.stream.height,
                                                   settings.stream.fps),
                          settings_effective_bitrate(&settings, NULL));
}

void testOptimalBitrateFitsSlider() {
    // on_res_fps_updated feeds this into the slider, so an uncapped result must still be clamped
    // by the caller rather than silently landing on the unlimited step.
    SS4S_VideoCapabilities no_caps = {0};
    TEST_ASSERT_GREATER_THAN_INT(BITRATE_MAX, settings_optimal_bitrate(&no_caps, 3840, 2160, 144));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(testReadINI);
    RUN_TEST(testWriteINI);
    RUN_TEST(testUnlimitedBitrateRoundTrip);
    RUN_TEST(testEffectiveBitrate);
    RUN_TEST(testOptimalBitrateFitsSlider);
    return UNITY_END();
}