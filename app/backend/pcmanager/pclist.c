#include "priv.h"

void pcmanager_list_lock(pcmanager_t *manager) {
    SDL_LockMutex(manager->servers_lock);
}

void pcmanager_list_unlock(pcmanager_t *manager) {
    SDL_UnlockMutex(manager->servers_lock);
}

void serverlist_set_from_resp(PSERVER_LIST node, PPCMANAGER_RESP resp) {
    if (resp->state.code != SERVER_STATE_NONE) {
        SDL_memcpy(&node->state, &resp->state, sizeof(resp->state));
    }
    node->known = resp->known;
    node->server = resp->server;
    resp->server_referenced = true;
}

void serverlist_nodefree(PSERVER_LIST node) {
    if (node->server) {
        serverdata_free((PSERVER_DATA) node->server);
    }
    free(node);
}