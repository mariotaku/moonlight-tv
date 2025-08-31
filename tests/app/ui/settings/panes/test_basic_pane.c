#include "unity.h"
#include "ui/settings/panes/basic.pane.c"

void setUp(void) {
    // Set up before each test
}

void tearDown(void) {
    // Clean up after each test
}

void test_max_resolution_uhd(void) {
    int mw = 4096, mh = 2304, nw = 3840, nh = 2160;
    calculate_max_resolution(1920, 1080, &mw, &mh, &nw, &nh);
    TEST_ASSERT_EQUAL(4096, mw);
    TEST_ASSERT_EQUAL(2304, mh);
    TEST_ASSERT_EQUAL(3840, nw);
    TEST_ASSERT_EQUAL(2160, nh);
}

void test_max_resolution_ultrawide(void) {
    int mw = 4096, mh = 2304, nw = 1920, nh = 1080;
    calculate_max_resolution(1720, 720, &mw, &mh, &nw, &nh);
    TEST_ASSERT_EQUAL(4096, mw);
    TEST_ASSERT_EQUAL(2304, mh);
    TEST_ASSERT_EQUAL(3440, nw);
    TEST_ASSERT_EQUAL(1440, nh);
}

void test_no_max_resolution_ultrawide(void) {
    int mw = 0, mh = 0, nw = 1920, nh = 1080;
    calculate_max_resolution(1720, 720, &mw, &mh, &nw, &nh);
    TEST_ASSERT_EQUAL(3440, mw);
    TEST_ASSERT_EQUAL(1440, mh);
    TEST_ASSERT_EQUAL(3440, nw);
    TEST_ASSERT_EQUAL(1440, nh);
}

void test_no_max_resolution_ultrawide_ui2x(void) {
    int mw = 0, mh = 0, nw = 1920, nh = 1080;
    calculate_max_resolution(3440, 1440, &mw, &mh, &nw, &nh);
    TEST_ASSERT_EQUAL(3440, mw);
    TEST_ASSERT_EQUAL(1440, mh);
    TEST_ASSERT_EQUAL(3440, nw);
    TEST_ASSERT_EQUAL(1440, nh);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_max_resolution_uhd);
    RUN_TEST(test_max_resolution_ultrawide);
    RUN_TEST(test_no_max_resolution_ultrawide);
    RUN_TEST(test_no_max_resolution_ultrawide_ui2x);
    return UNITY_END();
}
