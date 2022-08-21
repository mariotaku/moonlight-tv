
#include "backend/pcmanager.h"
#include "priv.h"

#include "pclist.h"
#include "app.h"
#include "backend/pcmanager/worker/worker.h"

static void pcmanager_free(executor_t *executor);

pcmanager_t *pcmanager_new() {
    pcmanager_t *manager = SDL_calloc(1, sizeof(pcmanager_t));
    manager->executor = executor_create("pcmanager", pcmanager_free);
    manager->thread_id = SDL_ThreadID();
    manager->servers_lock = SDL_CreateMutex();
    executor_set_userdata(manager->executor, manager);
    pcmanager_load_known_hosts(manager);
    return manager;
}

void pcmanager_destroy(pcmanager_t *manager) {
    pcmanager_auto_discovery_stop(manager);
    executor_destroy(manager->executor, 1);
    pcmanager_save_known_hosts(manager);
    pclist_free(manager);
    SDL_DestroyMutex(manager->servers_lock);
    SDL_free(manager);
}

bool pcmanager_quitapp(pcmanager_t *manager, const uuidstr_t *uuid, pcmanager_callback_t callback, void *userdata) {
    if (pcmanager_server_current_app(pcmanager, uuid) == 0) {
        return false;
    }
    cm_request_t *req = worker_context_new(manager, uuid, callback, userdata);
    pcmanager_worker_queue(manager, worker_quit_app, req);
    return true;
}

void pcmanager_request_update(pcmanager_t *manager, const uuidstr_t *uuid, pcmanager_callback_t callback,
                              void *userdata) {
    cm_request_t *req = worker_context_new(manager, uuid, callback, userdata);
    pcmanager_worker_queue(manager, worker_host_update, req);
}

void pcmanager_favorite_app(pcmanager_t *manager, const uuidstr_t *uuid, int appid, bool favorite) {
    pclist_lock(manager);
    pclist_t *node = pclist_find_by_uuid(manager, uuid);
    if (!node) {
        goto unlock;
    }
    pclist_node_set_app_favorite(node, appid, favorite);
    unlock:
    pclist_unlock(manager);
}

bool pcmanager_is_favorite(pcmanager_t *manager, const uuidstr_t *uuid, int appid) {
    const pclist_t *node = pcmanager_node(manager, uuid);
    if (!node) {
        return false;
    }
    return pcmanager_node_is_app_favorite(node, appid);
}

bool pcmanager_select(pcmanager_t *manager, const uuidstr_t *uuid) {
    pclist_lock(manager);
    const pclist_t *node = pcmanager_node(manager, uuid);
    if (node == NULL) {
        pclist_unlock(manager);
        return false;
    }
    for (pclist_t *cur = pcmanager->servers; cur; cur = cur->next) {
        cur->selected = node == cur;
    }
    pclist_unlock(manager);
    return true;
}

bool pcmanager_forget(pcmanager_t *manager, const uuidstr_t *uuid) {
    const pclist_t *node = pcmanager_node(manager, uuid);
    if (node == NULL) {
        return false;
    }
    pclist_remove(manager, uuid);
    return true;
}

const pclist_t *pcmanager_node(pcmanager_t *manager, const uuidstr_t *uuid) {
    return pclist_find_by_uuid(manager, uuid);
}

const SERVER_STATE *pcmanager_state(pcmanager_t *manager, const uuidstr_t *uuid) {
    const pclist_t *node = pcmanager_node(manager, uuid);
    if (!node) {
        return NULL;
    }
    return &node->state;
}

int pcmanager_server_current_app(pcmanager_t *manager, const uuidstr_t *uuid) {
    const pclist_t *node = pcmanager_node(manager, uuid);
    if (!node) {
        return 0;
    }
    return pcmanager_node_current_app(node);
}

int pcmanager_node_current_app(const pclist_t *node) {
    SDL_assert(node != NULL);
    return node->server->currentGame;
}

bool pcmanager_send_wol(pcmanager_t *manager, const uuidstr_t *uuid, pcmanager_callback_t callback,
                        void *userdata) {
    cm_request_t *req = worker_context_new(manager, uuid, callback, userdata);
    pcmanager_worker_queue(manager, worker_wol, req);
    return true;
}

const pclist_t *pcmanager_servers(pcmanager_t *manager) {
    return manager->servers;
}

static void pcmanager_free(executor_t *executor) {

}