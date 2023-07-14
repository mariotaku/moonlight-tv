#include "priv.h"
#include "client.h"
#include "util/bus.h"
#include "logging.h"
#include "backend/pcmanager/worker/worker.h"

static int pin_random(int min, int max);

bool pcmanager_pair(pcmanager_t *manager, const uuidstr_t *uuid, char *pin, pcmanager_callback_t callback,
                    void *userdata) {
    SDL_assert_release(manager != NULL);
    SDL_assert_release(uuid != NULL);
    SDL_assert_release(pin != NULL);
    const pclist_t *node = pcmanager_node(manager, uuid);
    if (node == NULL) {
        return false;
    }
    SERVER_DATA *server = node->server;
    if (server->paired) return false;
    if (server->currentGame) {
        commons_log_info("PCManager", "The server %s is in game", server->hostname);
    }
    int pin_num = pin_random(0, 9999);
    SDL_snprintf(pin, 5, "%04d", pin_num);
    worker_context_t *ctx = worker_context_new(manager, uuid, callback, userdata);
    ctx->arg1 = strdup(pin);
    pcmanager_worker_queue(manager, worker_pairing, ctx);
    return true;
}

bool pcmanager_manual_add(pcmanager_t *manager, const char *address, pcmanager_callback_t callback, void *userdata) {
    worker_context_t *ctx = worker_context_new(manager, NULL, callback, userdata);
    ctx->arg1 = strdup(address);
    pcmanager_worker_queue(manager, worker_add_by_ip, ctx);
    return true;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantParameter"
#pragma ide diagnostic ignored "cert-msc50-cpp"

static int pin_random(int min, int max) {
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

#pragma clang diagnostic pop
