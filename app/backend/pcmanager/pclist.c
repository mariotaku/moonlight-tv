#include "pclist.h"
#include "priv.h"

#include <errors.h>
#include <assert.h>
#include <util/bus.h>

typedef struct {
    pcmanager_t *manager;
    const pcmanager_resp_t *resp;
    SDL_mutex *mutex;
    SDL_cond *cond;
    bool sent;
} upsert_args_t;

#define LINKEDLIST_IMPL
#define LINKEDLIST_MODIFIER static
#define LINKEDLIST_TYPE SERVER_LIST
#define LINKEDLIST_PREFIX serverlist
#define LINKEDLIST_DOUBLE 1

#include "util/linked_list.h"
#include "listeners.h"

#undef LINKEDLIST_DOUBLE
#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX

static void upsert_perform(upsert_args_t *args);

static void upsert_wait(upsert_args_t *args);

static void serverlist_nodefree(PSERVER_LIST node);


SERVER_LIST *pclist_insert_known(pcmanager_t *manager, SERVER_DATA *server) {
    SERVER_LIST *node = serverlist_new();
    node->state.code = SERVER_STATE_NONE;
    node->server = server;
    node->known = true;
    manager->servers = serverlist_append(manager->servers, node);
    return node;
}

void pclist_upsert(pcmanager_t *manager, const pcmanager_resp_t *resp) {
    assert(manager);
    assert(resp);
    upsert_args_t *args = SDL_malloc(sizeof(upsert_args_t));
    args->manager = manager;
    args->resp = resp;
    args->sent = false;
    args->cond = NULL;
    if (SDL_ThreadID() == manager->thread_id) {
        upsert_perform(args);
    } else {
        args->mutex = SDL_CreateMutex();
        args->cond = SDL_CreateCond();
        bus_pushaction((bus_actionfunc) upsert_perform, args);
        upsert_wait(args);
        SDL_DestroyCond(args->cond);
        SDL_DestroyMutex(args->mutex);
    }
    free(args);
}

void pclist_free(pcmanager_t *manager) {
    serverlist_free(manager->servers, serverlist_nodefree);
}


void pcmanager_list_lock(pcmanager_t *manager) {
    SDL_LockMutex(manager->servers_lock);
}

void pcmanager_list_unlock(pcmanager_t *manager) {
    SDL_UnlockMutex(manager->servers_lock);
}

void pclist_node_apply(PSERVER_LIST node, const pcmanager_resp_t* resp) {
    assert(resp->server);
    if (resp->state.code != SERVER_STATE_NONE) {
        memcpy(&node->state, &resp->state, sizeof(SERVER_STATE));
    }
    node->server = resp->server;
    node->known |= resp->server->paired;
}

void serverlist_nodefree(PSERVER_LIST node) {
    if (node->server) {
        serverdata_free((PSERVER_DATA) node->server);
    }
    free(node);
}


static int serverlist_find_address(PSERVER_LIST other, const void *v) {
    return SDL_strcmp(other->server->serverInfo.address, (char *) v);
}

int serverlist_compare_uuid(PSERVER_LIST other, const void *v) {
    return SDL_strcasecmp(v, other->server->uuid);
}

PSERVER_LIST pcmanager_find_by_address(pcmanager_t *manager, const char *srvaddr) {
    SDL_assert(srvaddr);
    return serverlist_find_by(manager->servers, srvaddr, serverlist_find_address);
}

void upsert_perform(upsert_args_t *args) {
    pcmanager_t *manager = args->manager;
    const pcmanager_resp_t *resp = args->resp;
    SERVER_LIST *node = serverlist_find_by(manager->servers, resp->server->uuid, serverlist_compare_uuid);
    bool updated = node != NULL;
    if (!node) {
        node = serverlist_new();
        manager->servers = serverlist_append(manager->servers, node);
    }
    pclist_node_apply(node, resp);
    pcmanager_listeners_notify(manager, resp, updated);
    if (!args->cond) return;
    SDL_CondSignal(args->cond);
}

void upsert_wait(upsert_args_t *args) {
    while (!args->sent) {
        SDL_LockMutex(args->mutex);
        SDL_CondWait(args->cond, args->mutex);
        SDL_UnlockMutex(args->mutex);
    }
}