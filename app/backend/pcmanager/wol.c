#include "backend/pcmanager.h"

#include <errno.h>
#include <string.h>

#if __WIN32
#else

#include <arpa/inet.h>

#endif

#include <SDL.h>

#include "app.h"
#include "priv.h"
#include "util/logging.h"

static int pcmanager_send_wol_action(cm_request_t *req);

static void pcmanager_send_wol_cleanup(cm_request_t *req);

static bool wol_build_packet(const char *macstr, uint8_t *packet);

bool pcmanager_send_wol(pcmanager_t *manager, const char *uuid, pcmanager_callback_t callback,
                        void *userdata) {
    cm_request_t *req = cm_request_new(manager, serverdata_clone(server), callback, userdata);
    executor_execute(manager->executor, (executor_action_cb) pcmanager_send_wol_action,
                     (executor_cleanup_cb) pcmanager_send_wol_cleanup, req);
    return true;
}

static int pcmanager_send_wol_action(cm_request_t *req) {
    int ret = 0;
#ifndef __WIN32
    uint8_t packet[102];
    if (!wol_build_packet((char *) req->server->mac, packet)) {
        ret = -1;
        goto finish;
    }
    int broadcast = 1;
    int sockfd;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast)) {
        applog_e("WoL", "setsockopt() error: %d %s", errno, strerror(errno));
        ret = -1;
        goto finish;
    }

    struct sockaddr_in client, server;
    client.sin_family = AF_INET;
    client.sin_addr.s_addr = INADDR_ANY;
    client.sin_port = 0;
    // Bind socket
    if (bind(sockfd, (struct sockaddr *) &client, sizeof(client))) {
        applog_e("WoL", "bind() error: %d %s", errno, strerror(errno));
        ret = -1;
        goto finish;
    }

    // Set server endpoint (broadcast)
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("255.255.255.255");
    server.sin_port = htons(9);

    sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *) &server, sizeof(server));
    if (errno) {
        applog_e("WoL", "sendto() error: %d %s", errno, strerror(errno));
        ret = -1;
        goto finish;
    }

#endif
    finish:
    {
        Uint32 timeout = SDL_GetTicks() + 15000;
        GS_CLIENT gs = app_gs_client_new();
        while (!SDL_TICKS_PASSED(SDL_GetTicks(), timeout)) {
            PSERVER_DATA tmpserver = serverdata_new();
            ret = gs_init(gs, tmpserver, strdup(req->server->serverInfo.address), false);
            serverdata_free(tmpserver);
            applog_d("WoL", "gs_init returned %d, errno=%d", (int) ret, errno);
            if (ret == 0 || errno == ECONNREFUSED) {
                break;
            }
            SDL_Delay(3000);
        }
        gs_destroy(gs);
        pcmanager_resp_t *resp = serverinfo_resp_new();
        resp->server = req->server;
        resp->result.code = ret;
        pcmanager_worker_finalize(resp, req->callback, req->userdata);
    }
    return ret;
}

static void pcmanager_send_wol_cleanup(cm_request_t *req) {
    serverdata_free((SERVER_DATA *) req->server);
    free(req);
}

static bool wol_build_packet(const char *macstr, uint8_t *packet) {
    unsigned int values[6];
    if (sscanf(macstr, "%x:%x:%x:%x:%x:%x%*c", &values[0], &values[1], &values[2], &values[3], &values[4],
               &values[5]) != 6) {
        return false;
    }
    uint8_t mac[6];
    for (int i = 0; i < 6; i++) {
        mac[i] = (uint8_t) values[i];
    }
    memset(packet, 0xFF, 6);
    for (int i = 0; i < 16; i++) {
        memcpy(&packet[6 + i * 6], mac, 6);
    }
    return true;
}