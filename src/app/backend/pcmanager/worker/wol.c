#include "worker.h"
#include "app.h"
#include "logging.h"
#include "../pclist.h"
#include "../priv.h"

#include "util/wol.h"

#include <errno.h>
#include <SDL.h>

int worker_wol(worker_context_t *context) {
    const pclist_t *node = pcmanager_node(context->manager, &context->uuid);
    if (node == NULL) {
        pcmanager_unlock(context->manager);
        return ENOENT;
    }
    SERVER_DATA *server = node->server;
    wol_broadcast(server->mac);
    Uint32 timeout = SDL_GetTicks() + 15000;
    GS_CLIENT gs = app_gs_client_new(context->app);
    int ret;
    while (!SDL_TICKS_PASSED(SDL_GetTicks(), timeout)) {
        PSERVER_DATA tmpserver = serverdata_new();
        ret = gs_get_status(gs, tmpserver, strdup(server->serverInfo.address), false);
        serverdata_free(tmpserver);
        commons_log_debug("WoL", "gs_get_status returned %d, errno=%d", (int) ret, errno);
        if (ret == 0 || errno == ECONNREFUSED) {
            break;
        }
        SDL_Delay(3000);
    }
    gs_destroy(gs);
    return ret;
}
