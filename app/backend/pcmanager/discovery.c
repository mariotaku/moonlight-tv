#include "backend/computer_manager.h"

#include <stdio.h>

#include "mdns.h"

#include <errno.h>
#include <netdb.h>
#include <ifaddrs.h>

#include <pthread.h>

#include "libgamestream/errors.h"
#include "stream/settings.h"

#include "util/bus.h"
#include "util/user_event.h"

#include "app.h"
#include "priv.h"

#include "util/memlog.h"

#define SERVICE_NAME "_nvstream._tcp.local"

static char addrbuffer[64];
static char entrybuffer[256];
static char namebuffer[256];
static char sendbuffer[256];
static mdns_record_txt_t txtbuffer[128];

static uint32_t service_address_ipv4;
static uint8_t service_address_ipv6[16];

static int has_ipv4;
static int has_ipv6;

typedef struct
{
    const char *service;
    const char *hostname;
    uint32_t address_ipv4;
    uint8_t *address_ipv6;
    int port;
} service_record_t;

bool computer_discovery_running = false;

static mdns_string_t
ipv4_address_to_string(char *buffer, size_t capacity, const struct sockaddr_in *addr,
                       size_t addrlen)
{
    char host[NI_MAXHOST] = {0};
    char service[NI_MAXSERV] = {0};
    int ret = getnameinfo((const struct sockaddr *)addr, (socklen_t)addrlen, host, NI_MAXHOST,
                          service, NI_MAXSERV, NI_NUMERICSERV | NI_NUMERICHOST);
    int len = 0;
    if (ret == 0)
    {
        if (addr->sin_port != 0)
            len = snprintf(buffer, capacity, "%s:%s", host, service);
        else
            len = snprintf(buffer, capacity, "%s", host);
    }
    if (len >= (int)capacity)
        len = (int)capacity - 1;
    mdns_string_t str;
    str.str = buffer;
    str.length = len;
    return str;
}

static mdns_string_t
ipv6_address_to_string(char *buffer, size_t capacity, const struct sockaddr_in6 *addr,
                       size_t addrlen)
{
    char host[NI_MAXHOST] = {0};
    char service[NI_MAXSERV] = {0};
    int ret = getnameinfo((const struct sockaddr *)addr, (socklen_t)addrlen, host, NI_MAXHOST,
                          service, NI_MAXSERV, NI_NUMERICSERV | NI_NUMERICHOST);
    int len = 0;
    if (ret == 0)
    {
        if (addr->sin6_port != 0)
            len = snprintf(buffer, capacity, "[%s]:%s", host, service);
        else
            len = snprintf(buffer, capacity, "%s", host);
    }
    if (len >= (int)capacity)
        len = (int)capacity - 1;
    mdns_string_t str;
    str.str = buffer;
    str.length = len;
    return str;
}

static mdns_string_t
ip_address_to_string(char *buffer, size_t capacity, const struct sockaddr *addr, size_t addrlen)
{
    if (addr->sa_family == AF_INET6)
        return ipv6_address_to_string(buffer, capacity, (const struct sockaddr_in6 *)addr, addrlen);
    return ipv4_address_to_string(buffer, capacity, (const struct sockaddr_in *)addr, addrlen);
}

static char *parse_server_name(mdns_string_t entrystr)
{
    static const char suffix[] = ".local.";
    int nlen = entrystr.length;
    if (entrystr.length > 7 && strncmp(&(entrystr.str[entrystr.length - 7]), suffix, 7) == 0)
    {
        nlen -= 7;
    }
    char *srvname = calloc(nlen + 1, sizeof(char));
    snprintf(srvname, nlen + 1, "%.*s", nlen, entrystr.str);
    return srvname;
}

static int
query_callback(int sock, const struct sockaddr *from, size_t addrlen, mdns_entry_type_t entry,
               uint16_t query_id, uint16_t rtype, uint16_t rclass, uint32_t ttl, const void *data,
               size_t size, size_t name_offset, size_t name_length, size_t record_offset,
               size_t record_length, void *user_data)
{
    (void)sizeof(sock);
    (void)sizeof(query_id);
    (void)sizeof(name_length);
    (void)sizeof(user_data);
    mdns_string_t fromaddrstr = ip_address_to_string(addrbuffer, sizeof(addrbuffer), from, addrlen);
    const char *entrytype = (entry == MDNS_ENTRYTYPE_ANSWER) ? "answer" : ((entry == MDNS_ENTRYTYPE_AUTHORITY) ? "authority" : "additional");
    mdns_string_t entrystr =
        mdns_string_extract(data, size, &name_offset, entrybuffer, sizeof(entrybuffer));
    if (rtype == MDNS_RECORDTYPE_A)
    {
        struct sockaddr_in addr;
        mdns_record_parse_a(data, size, record_offset, record_length, &addr);
        mdns_string_t addrstr =
            ipv4_address_to_string(namebuffer, sizeof(namebuffer), &addr, sizeof(addr));
        char *srvaddr = calloc(addrstr.length + 1, sizeof(char));
        snprintf(srvaddr, addrstr.length + 1, "%.*s", MDNS_STRING_FORMAT(addrstr));
        pcmanager_insert_by_address(srvaddr, false);
    }
    return 0;
}

static int
open_client_sockets(int *sockets, int max_sockets, int port)
{
    // When sending, each socket can only send to one network interface
    // Thus we need to open one socket for each interface and address family
    int num_sockets = 0;

    struct ifaddrs *ifaddr = 0;
    struct ifaddrs *ifa = 0;

    if (getifaddrs(&ifaddr) < 0)
        fprintf(stderr, "Unable to get interface addresses\n");

    int first_ipv4 = 1;
    int first_ipv6 = 1;
    for (ifa = ifaddr; ifa; ifa = ifa->ifa_next)
    {
        if (!ifa->ifa_addr)
            continue;

        if (ifa->ifa_addr->sa_family == AF_INET)
        {
            struct sockaddr_in *saddr = (struct sockaddr_in *)ifa->ifa_addr;
            if (saddr->sin_addr.s_addr != htonl(INADDR_LOOPBACK))
            {
                int log_addr = 0;
                if (first_ipv4)
                {
                    service_address_ipv4 = saddr->sin_addr.s_addr;
                    first_ipv4 = 0;
                    log_addr = 1;
                }
                has_ipv4 = 1;
                if (num_sockets < max_sockets)
                {
                    saddr->sin_port = htons(port);
                    int sock = mdns_socket_open_ipv4(saddr);
                    if (sock >= 0)
                    {
                        sockets[num_sockets++] = sock;
                        log_addr = 1;
                    }
                    else
                    {
                        log_addr = 0;
                    }
                }
            }
        }
        else if (ifa->ifa_addr->sa_family == AF_INET6)
        {
            struct sockaddr_in6 *saddr = (struct sockaddr_in6 *)ifa->ifa_addr;
            static const unsigned char localhost[] = {0, 0, 0, 0, 0, 0, 0, 0,
                                                      0, 0, 0, 0, 0, 0, 0, 1};
            static const unsigned char localhost_mapped[] = {0, 0, 0, 0, 0, 0, 0, 0,
                                                             0, 0, 0xff, 0xff, 0x7f, 0, 0, 1};
            if (memcmp(saddr->sin6_addr.s6_addr, localhost, 16) &&
                memcmp(saddr->sin6_addr.s6_addr, localhost_mapped, 16))
            {
                int log_addr = 0;
                if (first_ipv6)
                {
                    memcpy(service_address_ipv6, &saddr->sin6_addr, 16);
                    first_ipv6 = 0;
                    log_addr = 1;
                }
                has_ipv6 = 1;
                if (num_sockets < max_sockets)
                {
                    saddr->sin6_port = htons(port);
                    int sock = mdns_socket_open_ipv6(saddr);
                    if (sock >= 0)
                    {
                        sockets[num_sockets++] = sock;
                        log_addr = 1;
                    }
                    else
                    {
                        log_addr = 0;
                    }
                }
            }
        }
    }

    freeifaddrs(ifaddr);

    return num_sockets;
}

void *_computer_manager_polling_action(void *data)
{
    computer_discovery_running = true;
#if _GNU_SOURCE
    pthread_setname_np(pthread_self(), "hostscan");
#endif
    int sockets[32];
    int query_id[32];
    int num_sockets = open_client_sockets(sockets, sizeof(sockets) / sizeof(sockets[0]), 0);
    if (num_sockets <= 0)
    {
        fprintf(stderr, "Failed to open any client sockets\n");
        computer_discovery_running = false;
        return NULL;
    }

    size_t capacity = 2048;
    void *buffer = malloc(capacity);
    void *user_data = 0;
    size_t records;

    for (int isock = 0; isock < num_sockets; ++isock)
    {
        query_id[isock] = mdns_query_send(sockets[isock], MDNS_RECORDTYPE_PTR, SERVICE_NAME,
                                          sizeof(SERVICE_NAME) - 1, buffer, capacity, 0);
        if (query_id[isock] < 0)
            fprintf(stderr, "Failed to send mDNS query: %s\n", strerror(errno));
    }

    // This is a simple implementation that loops for 5 seconds or as long as we get replies
    int res;
    do
    {
        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        int nfds = 0;
        fd_set readfs;
        FD_ZERO(&readfs);
        for (int isock = 0; isock < num_sockets; ++isock)
        {
            if (sockets[isock] >= nfds)
                nfds = sockets[isock] + 1;
            FD_SET(sockets[isock], &readfs);
        }

        records = 0;
        res = select(nfds, &readfs, 0, 0, &timeout);
        if (res > 0)
        {
            for (int isock = 0; isock < num_sockets; ++isock)
            {
                if (FD_ISSET(sockets[isock], &readfs))
                {
                    records += mdns_query_recv(sockets[isock], buffer, capacity, query_callback,
                                               user_data, query_id[isock]);
                }
                FD_SET(sockets[isock], &readfs);
            }
        }
    } while (res > 0);

    free(buffer);

    for (int isock = 0; isock < num_sockets; ++isock)
        mdns_socket_close(sockets[isock]);
    computer_discovery_running = false;
    return NULL;
}

int pcmanager_insert_by_address(char *srvaddr, bool pair)
{
    PSERVER_DATA server = malloc(sizeof(SERVER_DATA));
    memset(server, 0, sizeof(SERVER_DATA));
    int ret = gs_init(server, srvaddr, app_configuration->key_dir, app_configuration->debug_level, app_configuration->unsupported);

    PSERVER_LIST node = serverlist_new();
    node->err = ret;
    if (ret == GS_OK)
    {
        if (server->paired)
        {
            node->known = true;
        }
        node->server = server;
        node->errmsg = NULL;
    }
    else
    {
        node->server = NULL;
        node->errmsg = gs_error;
        free(server);
    }
    bus_pushevent(USER_CM_SERVER_DISCOVERED, node, (void *)pair);
    return ret;
}