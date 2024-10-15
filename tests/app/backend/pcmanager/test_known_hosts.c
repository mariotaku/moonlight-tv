#include "unity.h"
#include "backend/pcmanager/known_hosts.h"
#include "hostport.h"

void setUp(void) {

}

void tearDown(void) {

}

void test_parse() {
    known_host_t *hosts = known_hosts_parse(FIXTURES_PATH_PREFIX "hosts_read.ini");
    TEST_ASSERT_EQUAL_STRING("192.168.1.100", hostport_get_hostname(hosts->address));
    TEST_ASSERT_EQUAL(0, hostport_get_port(hosts->address));

    known_host_t *next = hosts->next;
    TEST_ASSERT_EQUAL_STRING("192.168.1.101", hostport_get_hostname(next->address));
    TEST_ASSERT_EQUAL(47985, hostport_get_port(next->address));

    known_hosts_free(hosts, known_hosts_node_free);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_parse);
    return UNITY_END();
}