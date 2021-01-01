#include "computer_manager.h"

#include <stdio.h>

#include "mdns.h"

#include <errno.h>

#include <netdb.h>
#include <ifaddrs.h>

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
    if (rtype == MDNS_RECORDTYPE_PTR)
    {
        mdns_string_t namestr = mdns_record_parse_ptr(data, size, record_offset, record_length,
                                                      namebuffer, sizeof(namebuffer));
        fprintf(stderr, "%.*s : %s %.*s PTR %.*s rclass 0x%x ttl %u length %d\n",
                MDNS_STRING_FORMAT(fromaddrstr), entrytype, MDNS_STRING_FORMAT(entrystr),
                MDNS_STRING_FORMAT(namestr), rclass, ttl, (int)record_length);
    }
    else if (rtype == MDNS_RECORDTYPE_SRV)
    {
        mdns_record_srv_t srv = mdns_record_parse_srv(data, size, record_offset, record_length,
                                                      namebuffer, sizeof(namebuffer));
        fprintf(stderr, "%.*s : %s %.*s SRV %.*s priority %d weight %d port %d\n",
                MDNS_STRING_FORMAT(fromaddrstr), entrytype, MDNS_STRING_FORMAT(entrystr),
                MDNS_STRING_FORMAT(srv.name), srv.priority, srv.weight, srv.port);
    }
    else if (rtype == MDNS_RECORDTYPE_A)
    {
        struct sockaddr_in addr;
        mdns_record_parse_a(data, size, record_offset, record_length, &addr);
        mdns_string_t addrstr =
            ipv4_address_to_string(namebuffer, sizeof(namebuffer), &addr, sizeof(addr));
        NVCOMPUTER *computer = malloc(sizeof(NVCOMPUTER));
        snprintf(computer->name, addrstr.length + 1, "%.*s", MDNS_STRING_FORMAT(addrstr));

        _computer_manager_add(computer);
        // fprintf(stderr, "%.*s : %s %.*s A %.*s\n", MDNS_STRING_FORMAT(fromaddrstr), entrytype,
        //         MDNS_STRING_FORMAT(entrystr), MDNS_STRING_FORMAT(addrstr));
    }
    // Wait for response from original author: https://github.com/mjansson/mdns/issues/36
    // else if (rtype == MDNS_RECORDTYPE_AAAA)
    // {
    //     struct sockaddr_in6 addr;
    //     mdns_record_parse_aaaa(data, size, record_offset, record_length, &addr);
    //     mdns_string_t addrstr =
    //         ipv6_address_to_string(namebuffer, sizeof(namebuffer), &addr, sizeof(addr));
    //     fprintf(stderr, "%.*s : %s %.*s AAAA %.*s\n", MDNS_STRING_FORMAT(fromaddrstr), entrytype,
    //            MDNS_STRING_FORMAT(entrystr), MDNS_STRING_FORMAT(addrstr));
    // }
    else if (rtype == MDNS_RECORDTYPE_TXT)
    {
        size_t parsed = mdns_record_parse_txt(data, size, record_offset, record_length, txtbuffer,
                                              sizeof(txtbuffer) / sizeof(mdns_record_txt_t));
        for (size_t itxt = 0; itxt < parsed; ++itxt)
        {
            if (txtbuffer[itxt].value.length)
            {
                fprintf(stderr, "%.*s : %s %.*s TXT %.*s = %.*s\n", MDNS_STRING_FORMAT(fromaddrstr),
                        entrytype, MDNS_STRING_FORMAT(entrystr),
                        MDNS_STRING_FORMAT(txtbuffer[itxt].key),
                        MDNS_STRING_FORMAT(txtbuffer[itxt].value));
            }
            else
            {
                fprintf(stderr, "%.*s : %s %.*s TXT %.*s\n", MDNS_STRING_FORMAT(fromaddrstr), entrytype,
                        MDNS_STRING_FORMAT(entrystr), MDNS_STRING_FORMAT(txtbuffer[itxt].key));
            }
        }
    }
    else
    {
        fprintf(stderr, "%.*s : %s %.*s type %u rclass 0x%x ttl %u length %d\n",
                MDNS_STRING_FORMAT(fromaddrstr), entrytype, MDNS_STRING_FORMAT(entrystr), rtype,
                rclass, ttl, (int)record_length);
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
                if (log_addr)
                {
                    char buffer[128];
                    mdns_string_t addr = ipv4_address_to_string(buffer, sizeof(buffer), saddr,
                                                                sizeof(struct sockaddr_in));
                    fprintf(stderr, "Local IPv4 address: %.*s\n", MDNS_STRING_FORMAT(addr));
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
                if (log_addr)
                {
                    char buffer[128];
                    mdns_string_t addr = ipv6_address_to_string(buffer, sizeof(buffer), saddr,
                                                                sizeof(struct sockaddr_in6));
                    fprintf(stderr, "Local IPv6 address: %.*s\n", MDNS_STRING_FORMAT(addr));
                }
            }
        }
    }

    freeifaddrs(ifaddr);

    return num_sockets;
}

gpointer _computer_manager_polling_action(const gpointer data)
{
    const char *service = "_nvstream._tcp.local";
    int sockets[32];
    int query_id[32];
    int num_sockets = open_client_sockets(sockets, sizeof(sockets) / sizeof(sockets[0]), 0);
    if (num_sockets <= 0)
    {
        fprintf(stderr, "Failed to open any client sockets\n");
        return NULL;
    }
    fprintf(stderr, "Opened %d socket%s for mDNS query\n", num_sockets, num_sockets ? "s" : "");

    size_t capacity = 2048;
    void *buffer = malloc(capacity);
    void *user_data = 0;
    size_t records;

    fprintf(stderr, "Sending mDNS query: %s\n", service);
    for (int isock = 0; isock < num_sockets; ++isock)
    {
        query_id[isock] = mdns_query_send(sockets[isock], MDNS_RECORDTYPE_PTR, service,
                                          strlen(service), buffer, capacity, 0);
        if (query_id[isock] < 0)
            fprintf(stderr, "Failed to send mDNS query: %s\n", strerror(errno));
    }

    // This is a simple implementation that loops for 5 seconds or as long as we get replies
    int res;
    fprintf(stderr, "Reading mDNS query replies\n");
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
    fprintf(stderr, "Closed socket%s\n", num_sockets ? "s" : "");

    return NULL;
}