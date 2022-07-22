#include "pclist.h"
#include "priv.h"

#include <errors.h>
#include <assert.h>
#include <util/bus.h>

#include "listeners.h"

#define LINKEDLIST_IMPL
#define LINKEDLIST_MODIFIER static
#define LINKEDLIST_TYPE SERVER_LIST
#define LINKEDLIST_PREFIX serverlist
#define LINKEDLIST_DOUBLE 1

#include "util/linked_list.h"

#undef LINKEDLIST_DOUBLE
#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX

#define LINKEDLIST_IMPL
#define LINKEDLIST_MODIFIER static
#define LINKEDLIST_TYPE appid_list_t
#define LINKEDLIST_PREFIX favlist
#define LINKEDLIST_DOUBLE 1

#include "util/linked_list.h"

#undef LINKEDLIST_DOUBLE
#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX

static void upsert_perform(upsert_args_t *args);

static void remove_perform(upsert_args_t *args);

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
    upsert_args_t args = {.manager = manager, .resp = resp};
    if (SDL_ThreadID() == manager->thread_id) {
        upsert_perform(&args);
    } else {
        bus_pushaction_sync((bus_actionfunc) upsert_perform, &args);
    }
}

void pclist_remove(pcmanager_t *manager, const SERVER_DATA *server) {
    assert(manager);
    assert(server);
    pcmanager_resp_t resp = {
            .result.code = GS_OK,
            .server = server,
            .state.code = SERVER_STATE_NONE,
            .known = false,
    };
    upsert_args_t args = {.manager = manager, .resp = &resp};
    if (SDL_ThreadID() == manager->thread_id) {
        remove_perform(&args);
    } else {
        bus_pushaction_sync((bus_actionfunc) remove_perform, &args);
    }
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

static int favlist_find_id(appid_list_t *other, const void *v) {
    return other->id - *((const int *) v);
}

void pcmanager_favorite_app(SERVER_LIST *node, int appid, bool state) {
    appid_list_t *existing = favlist_find_by(node->favs, &appid, favlist_find_id);
    if (state) {
        if (existing) return;
        appid_list_t *item = favlist_new();
        item->id = appid;
        node->favs = favlist_append(node->favs, item);
    } else if (existing) {
        node->favs = favlist_remove(node->favs, existing);
    }
}

bool pcmanager_is_favorite(const SERVER_LIST *node, int appid) {
    return favlist_find_by(node->favs, &appid, favlist_find_id) != NULL;
}

void pclist_node_apply(PSERVER_LIST node, const pcmanager_resp_t *resp) {
    assert(resp->server);
    if (resp->state.code != SERVER_STATE_NONE) {
        memcpy(&node->state, &resp->state, sizeof(SERVER_STATE));
    }
    if (node->server && node->server != resp->server) {
        // Although it's const pointer, we free it and assign a new one.
        serverdata_free((SERVER_DATA *) node->server);
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

PSERVER_LIST pcmanager_find_by_uuid(pcmanager_t *manager, const char *uuid) {
    SDL_assert(uuid);
    return serverlist_find_by(manager->servers, uuid, serverlist_compare_uuid);
}

static void upsert_perform(upsert_args_t *args) {
    pcmanager_t *manager = args->manager;
    const pcmanager_resp_t *resp = args->resp;
    pcmanager_list_lock(manager);
    SERVER_LIST *node = serverlist_find_by(manager->servers, resp->server->uuid, serverlist_compare_uuid);
    bool updated = node != NULL;
    if (!node) {
        node = serverlist_new();
        manager->servers = serverlist_append(manager->servers, node);
    }
    pclist_node_apply(node, resp);
    pcmanager_list_unlock(manager);
    pcmanager_listeners_notify(manager, resp, updated ? PCMANAGER_NOTIFY_UPDATED : PCMANAGER_NOTIFY_ADDED);
}

static void remove_perform(upsert_args_t *args) {
    pcmanager_t *manager = args->manager;
    const pcmanager_resp_t *resp = args->resp;
    pcmanager_list_lock(manager);
    SERVER_LIST *node = serverlist_find_by(manager->servers, resp->server->uuid, serverlist_compare_uuid);
    if (!node) return;
    manager->servers = serverlist_remove(manager->servers, node);
    pcmanager_list_unlock(manager);
    pcmanager_listeners_notify(manager, resp, PCMANAGER_NOTIFY_REMOVED);
    serverlist_nodefree(node);
}

