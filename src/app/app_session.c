#include "app_session.h"
#include "app.h"
#include "stream/session.h"

int app_session_begin(app_t *app, const uuidstr_t *uuid, int app_id) {
    if (app->session != NULL) {
        return -1;
    }
    const pclist_t *node = pcmanager_node(pcmanager, uuid);
    if (node == NULL) {
        return -1;
    }
    app->session = session_create(app, app_configuration, node->server, app_id);
    return 0;
}

void app_session_destroy(app_t *app) {
    if (app->session == NULL) {
        return;
    }
    session_destroy(app->session);
    app->session = NULL;
}