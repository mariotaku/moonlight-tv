#include "unity.h"
#include "backend/pcmanager/known_hosts.h"

void setUp(void) {

}

void tearDown(void) {

}

void test_parse() {
    known_host_t *hosts = known_hosts_parse(FIXTURES_PATH_PREFIX "hosts_read.ini");
    TEST_ASSERT_EQUAL_STRING("192.168.1.100", hosts->address);
    TEST_ASSERT_EQUAL(0, hosts->port);

    known_host_t *next = hosts->next;
    TEST_ASSERT_EQUAL_STRING("192.168.1.101", next->address);
    TEST_ASSERT_EQUAL(47985, next->port);

    known_hosts_free(hosts, known_hosts_node_free);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_parse);
    return UNITY_END();
}