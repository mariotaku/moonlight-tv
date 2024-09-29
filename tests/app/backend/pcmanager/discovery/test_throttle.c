#include "unity.h"
#include "backend/pcmanager/discovery/throttle.h"

#include <SDL2/SDL_timer.h>

static discovery_throttle_t throttle;
static int counter = 0;

void callback(const sockaddr_t *addr, void *user_data) {
    (void) addr;
    (void) user_data;
    counter++;
}

void setUp(void) {
    counter = 0;
    discovery_throttle_init(&throttle, callback, NULL);
}

void tearDown(void) {
    discovery_throttle_deinit(&throttle);
}

void test_discovery_throttle(void) {
    sockaddr_t *addr = sockaddr_new();
    sockaddr_set_ip_str(addr, AF_INET, "192.168.1.110");
    sockaddr_set_port(addr, 47989);
    discovery_throttle_on_discovered(&throttle, addr, 10);
    discovery_throttle_on_discovered(&throttle, addr, 10);
    discovery_throttle_on_discovered(&throttle, addr, 10);
    TEST_ASSERT_EQUAL(1, counter);
    SDL_Delay(20);
    discovery_throttle_on_discovered(&throttle, addr, 10);
    TEST_ASSERT_EQUAL(2, counter);
    sockaddr_free(addr);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_discovery_throttle);
    return UNITY_END();
}