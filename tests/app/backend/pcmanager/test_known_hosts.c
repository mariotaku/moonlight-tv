#include "unity.h"
#include "backend/pcmanager/known_hosts.h"

void setUp(void) {

}

void tearDown(void) {

}

void test_parse() {
    char buf[64];
    known_host_t *hosts = known_hosts_parse(FIXTURES_PATH_PREFIX "hosts_read.ini");
    sockaddr_get_ip_str(hosts->address, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("192.168.1.100", buf);
    TEST_ASSERT_EQUAL(0, sockaddr_get_port(hosts->address));

    known_host_t *next = hosts->next;
    sockaddr_get_ip_str(next->address, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("192.168.1.101", buf);
    TEST_ASSERT_EQUAL(47985, sockaddr_get_port(next->address));

    known_hosts_free(hosts, known_hosts_node_free);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_parse);
    return UNITY_END();
}