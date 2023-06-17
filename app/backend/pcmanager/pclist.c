#include "pclist.h"
#include "priv.h"

#include <assert.h>
#include <util/bus.h>

#include "listeners.h"

#define LINKEDLIST_IMPL
#define LINKEDLIST_MODIFIER static
#define LINKEDLIST_TYPE pclist_t
#define LINKEDLIST_PREFIX pclist_ll
#define LINKEDLIST_DOUBLE 1

#include "util/linked_list.h"

#undef LINKEDLIST_DOUBLE
#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX

#define LINKEDLIST_IMPL
#define LINKEDLIST_MODIFIER static
#define LINKEDLIST_TYPE appid_list_t
#define LINKEDLIST_PREFIX favlist_ll
#define LINKEDLIST_DOUBLE 1

#include "util/linked_list.h"
#include "util/uuidstr.h"

#undef LINKEDLIST_DOUBLE
#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX

static void upsert_perform(pclist_update_context_t *context);

static void remove_perform(pclist_update_context_t *context);

static void pclist_ll_nodefree(pclist_t *node);

static int pclist_ll_compare_address(pclist_t *other, const void *v);

static int pclist_ll_compare_uuid(pclist_t *other, const void *v);

static int favlist_find_id(appid_list_t *other, const void *v);

pclist_t *pclist_insert_known(pcmanager_t *manager, const uuidstr_t *id, SERVER_DATA *server) {
    pclist_t *node = pclist_ll_new();
    node->id = *id;
    node->state.code = SERVER_STATE_NONE;
    node->server = server;
    node->known = true;
    manager->servers = pclist_ll_append(manager->servers, node);
    return node;
}

void pclist_upsert(pcmanager_t *manager, const uuidstr_t *uuid, const SERVER_STATE *state, SERVER_DATA *server) {
    assert(manager);
    assert(uuid);
    pclist_update_context_t args = {
            .manager = manager,
            .uuid = *uuid,
            .server = server,
            .state = *state
    };
    if (SDL_ThreadID() == manager->thread_id) {
        upsert_perform(&args);
    } else {
        bus_pushaction_sync((bus_actionfunc) upsert_perform, &args);
    }
}

/**
 * Thread safe.
 * @param manager
 * @param uuid
 */
void pclist_remove(pcmanager_t *manager, const uuidstr_t *uuid) {
    assert(manager);
    assert(uuid);
    pclist_update_context_t context = {
            .uuid = *uuid,
            .manager = manager,
    };
    if (SDL_ThreadID() == manager->thread_id) {
        remove_perform(&context);
    } else {
        bus_pushaction_sync((bus_actionfunc) remove_perform, &context);
    }
}

void pclist_free(pcmanager_t *manager) {
    pclist_ll_free(manager->servers, pclist_ll_nodefree);
}


void pclist_lock(pcmanager_t *manager) {
    SDL_LockMutex(manager->servers_lock);
}

void pclist_unlock(pcmanager_t *manager) {
    SDL_UnlockMutex(manager->servers_lock);
}

bool pcmanager_node_is_app_favorite(const pclist_t *node, int appid) {
    return favlist_ll_find_by(node->favs, &appid, favlist_find_id) != NULL;
}

bool pclist_node_set_app_favorite(pclist_t *node, int appid, bool favorite) {
    appid_list_t *existing = favlist_ll_find_by(node->favs, &appid, favlist_find_id);
    if (favorite) {
        if (existing) return false;
        appid_list_t *item = favlist_ll_new();
        item->id = appid;
        node->favs = favlist_ll_append(node->favs, item);
    } else if (existing) {
        node->favs = favlist_ll_remove(node->favs, existing);
    }
    return true;
}

void pclist_node_apply(pclist_t *node, const SERVER_STATE *state, SERVER_DATA *server) {
    if (state != NULL && state->code != SERVER_STATE_NONE) {
        node->state = *state;
    }
    if (server != NULL) {
        if (node->server != server) {
            if (node->server) {
                serverdata_free(node->server);
            }
            uuidstr_fromstr(&node->id, server->uuid);
            node->server = server;
        }
        node->known |= server->paired;
    }
}

void pclist_ll_nodefree(pclist_t *node) {
    if (node->server) {
        serverdata_free((PSERVER_DATA) node->server);
    }
    if (node->favs) {
        favlist_ll_free(node->favs, (favlist_ll_nodefree_fn) free);
    }
    free(node);
}


pclist_t *pclist_find_by_ip(pcmanager_t *manager, const char *ip) {
    SDL_assert_release(ip != NULL);
    pclist_lock(manager);
    pclist_t *result = pclist_ll_find_by(manager->servers, ip, pclist_ll_compare_address);
    pclist_unlock(manager);
    return result;
}

pclist_t *pclist_find_by_uuid(pcmanager_t *manager, const uuidstr_t *uuid) {
    SDL_assert_release(uuid != NULL);
    pclist_lock(manager);
    pclist_t *result = pclist_ll_find_by(manager->servers, uuid, pclist_ll_compare_uuid);
    pclist_unlock(manager);
    return result;
}

static int pclist_ll_compare_address(pclist_t *other, const void *v) {
    SDL_assert_release(v);
    SDL_assert_release(other);
    SDL_assert_release(other->server);
    SDL_assert_release(other->server->serverInfo.address);
    return SDL_strcmp(other->server->serverInfo.address, (const char *) v);
}

static int pclist_ll_compare_uuid(pclist_t *other, const void *v) {
    SDL_assert_release(v);
    SDL_assert_release(other);
    SDL_assert_release(other->server);
    return !uuidstr_t_equals_t(&other->id, (const uuidstr_t *) v);
}

static int favlist_find_id(appid_list_t *other, const void *v) {
    return other->id - *((const int *) v);
}

static void upsert_perform(pclist_update_context_t *context) {
    pcmanager_t *manager = context->manager;
    pclist_lock(manager);
    pclist_t *node = pclist_ll_find_by(manager->servers, &context->uuid, pclist_ll_compare_uuid);
    bool updated = node != NULL;
    if (!node) {
        node = pclist_ll_new();
        manager->servers = pclist_ll_append(manager->servers, node);
    }
    pclist_node_apply(node, &context->state, context->server);
    pclist_unlock(manager);
    pcmanager_listeners_notify(manager, &context->uuid, updated ? PCMANAGER_NOTIFY_UPDATED : PCMANAGER_NOTIFY_ADDED);
}

static void remove_perform(pclist_update_context_t *context) {
    pcmanager_t *manager = context->manager;
    pclist_lock(manager);
    pclist_t *node = pclist_ll_find_by(manager->servers, &context->uuid, pclist_ll_compare_uuid);
    if (!node) return;
    manager->servers = pclist_ll_remove(manager->servers, node);
    pclist_unlock(manager);
    pcmanager_listeners_notify(manager, &context->uuid, PCMANAGER_NOTIFY_REMOVED);
    pclist_ll_nodefree(node);
}