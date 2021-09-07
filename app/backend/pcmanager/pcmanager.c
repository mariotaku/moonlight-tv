#define PCMANAGER_IMPL

#include "backend/pcmanager.h"
#include "priv.h"

#include "app.h"

#include <assert.h>
#include <ctype.h>

#include <SDL.h>

#include <libconfig.h>

#include "libgamestream/errors.h"

#include "util/bus.h"
#include "util/path.h"

#include "stream/session.h"
#include "stream/settings.h"

#include "util/memlog.h"
#include "util/logging.h"
#include "util/libconfig_ext.h"

static void strlower(char *p) {
    for (; *p; ++p)
        *p = tolower(*p);
}

static SDL_Thread *computer_manager_polling_thread = NULL;
static PPCMANAGER_CALLBACKS callbacks_list;

PSERVER_LIST computer_list;

bool computer_discovery_running = false;

static int _pcmanager_quitapp_action(void *data);

static int _computer_manager_server_update_action(PSERVER_DATA data);

static void pcmanager_load_known_hosts();

static void pcmanager_save_known_hosts();

static void pcmanager_client_setup();

static void serverlist_set_from_resp(PSERVER_LIST node, PPCMANAGER_RESP resp);

bool pcmanager_setup_running = false;

void computer_manager_init() {
    computer_list = NULL;
    pcmanager_load_known_hosts();
    pcmanager_client_setup();
}

void computer_manager_destroy() {
    if (computer_discovery_running) {
        SDL_DetachThread(computer_manager_polling_thread);
        computer_manager_polling_thread = NULL;
    }
    pcmanager_save_known_hosts();
    serverlist_free(computer_list, serverlist_nodefree);
}

bool computer_manager_dispatch_userevent(int which, void *data1, void *data2) {
    return false;
}

void handle_server_updated(PPCMANAGER_RESP update) {
    assert(update);
    if (update->result.code != GS_OK || !update->server)
        return;
    PSERVER_LIST node = serverlist_find_by(computer_list, update->server->uuid, serverlist_compare_uuid);
    if (!node)
        return;
    if (update->server_shallow)
        free((PSERVER_DATA) node->server);
    else
        serverdata_free((PSERVER_DATA) node->server);

    serverlist_set_from_resp(node, update);
}

void handle_server_discovered(PPCMANAGER_RESP discovered) {
    if (discovered->state.code != SERVER_STATE_ONLINE)
        return;
    PSERVER_LIST node = serverlist_find_by(computer_list, discovered->server->uuid, serverlist_compare_uuid);
    if (node) {
        if (node->server) {
            serverdata_free((PSERVER_DATA) node->server);
        }
        serverlist_set_from_resp(node, discovered);
        for (PPCMANAGER_CALLBACKS cur = callbacks_list; cur != NULL; cur = cur->next) {
            if (!cur->updated) continue;
            cur->updated(cur->userdata, discovered);
        }
    } else {
        node = serverlist_new();
        serverlist_set_from_resp(node, discovered);

        computer_list = serverlist_append(computer_list, node);
        for (PPCMANAGER_CALLBACKS cur = callbacks_list; cur != NULL; cur = cur->next) {
            if (!cur->added) continue;
            cur->added(cur->userdata, discovered);
        }
    }
}

bool computer_manager_run_scan() {
    if (computer_discovery_running || streaming_status != STREAMING_NONE) {
        return false;
    }
    computer_manager_polling_thread = SDL_CreateThread(_computer_manager_polling_action, "hostscan", NULL);
    return true;
}

bool pcmanager_quitapp(const SERVER_DATA *server, pcmanager_callback_t callback, void *userdata) {
    if (server->currentGame == 0) {
        return false;
    }
    cm_request_t *req = malloc(sizeof(cm_request_t));
    req->server = server;
    req->callback = callback;
    req->userdata = userdata;
    SDL_CreateThread(_pcmanager_quitapp_action, "quitapp", req);
    return true;
}

int _pcmanager_quitapp_action(void *data) {
    cm_request_t *req = (cm_request_t *) data;
    PPCMANAGER_RESP resp = serverinfo_resp_new();
    PSERVER_DATA server = serverdata_new();
    SDL_memcpy(server, req->server, sizeof(SERVER_DATA));
    int ret = gs_quit_app(app_gs_client_obtain(), server);
    if (ret != GS_OK)
        pcmanager_resp_setgserror(resp, ret, gs_error);
    resp->server = server;
    resp->server_shallow = true;
    bus_pushaction((bus_actionfunc) handle_server_updated, resp);
    bus_pushaction((bus_actionfunc) invoke_callback, invoke_callback_args(resp, req->callback, req->userdata));
    bus_pushaction((bus_actionfunc) serverinfo_resp_free, resp);
    free(req);
    return 0;
}

int _computer_manager_server_update_action(PSERVER_DATA data) {
    PSERVER_DATA server = serverdata_new();
    PPCMANAGER_RESP update = serverinfo_resp_new();
    int ret = gs_init(app_gs_client_obtain(), server, SDL_strdup(data->serverInfo.address),
                      app_configuration->unsupported);
    if (ret == GS_OK) {
        update->state.code = SERVER_STATE_ONLINE;
        update->server = server;
        update->server_shallow = false;
    } else {
        serverstate_setgserror(&update->state, ret, gs_error);
    }
    bus_pushaction((bus_actionfunc) handle_server_updated, update);
    bus_pushaction((bus_actionfunc) serverinfo_resp_free, update);
    free(data);
    return 0;
}

int serverlist_compare_uuid(PSERVER_LIST other, const void *v) {
    return strcasecmp(v, other->server->uuid);
}

void pcmanager_load_known_hosts() {
    char *confdir = path_pref(), *conffile = path_join(confdir, CONF_NAME_HOSTS);
    free(confdir);
    config_t config;
    config_init(&config);
    int options = config_get_options(&config);
    options &= ~CONFIG_OPTION_OPEN_BRACE_ON_SEPARATE_LINE;
    options &= ~CONFIG_OPTION_COLON_ASSIGNMENT_FOR_GROUPS;
    config_set_options(&config, options);
    if (config_read_file(&config, conffile) != CONFIG_TRUE) {
        goto cleanup;
    }
    config_setting_t *root = config_root_setting(&config);
    bool selected_set = false;
    for (int i = 0, j = config_setting_length(root); i < j; i++) {
        config_setting_t *item = config_setting_get_elem(root, i);
        const char *mac = config_setting_get_string_simple(item, "mac"),
                *hostname = config_setting_get_string_simple(item, "hostname"),
                *address = config_setting_get_string_simple(item, "address");
        if (!mac || !hostname || !address) {
            continue;
        }
        const char *key = config_setting_name(item);
        int keyoff = key[0] == '*' ? 1 : 0;
        char *uuid = SDL_strdup(&key[keyoff]);
        PSERVER_DATA server = serverdata_new();
        server->uuid = uuid;
        server->mac = SDL_strdup(mac);
        server->hostname = SDL_strdup(hostname);
        server->serverInfo.address = SDL_strdup(address);

        PSERVER_LIST node = serverlist_new();
        node->state.code = SERVER_STATE_NONE;
        node->server = server;
        node->known = true;
        computer_list = serverlist_append(computer_list, node);
        if (!selected_set && config_setting_get_bool_simple(item, "selected")) {
            node->selected = true;
            selected_set = true;
        }
    }
    cleanup:
    config_destroy(&config);
    free(conffile);
}

void pcmanager_save_known_hosts() {
    config_t config;
    config_init(&config);
    int options = config_get_options(&config);
    options &= ~CONFIG_OPTION_OPEN_BRACE_ON_SEPARATE_LINE;
    options &= ~CONFIG_OPTION_COLON_ASSIGNMENT_FOR_GROUPS;
    config_set_options(&config, options);
    config_setting_t *root = config_root_setting(&config);
    bool selected_set = false;
    for (PSERVER_LIST cur = computer_list; cur != NULL; cur = cur->next) {
        if (!cur->server || !cur->known) {
            continue;
        }
        const SERVER_DATA *server = cur->server;
        char key[38];
        key[0] = '*';
        SDL_memcpy(&key[1], server->uuid, 36);
        key[37] = '\0';
        strlower(&key[1]);
        config_setting_t *item = config_setting_add(root, key, CONFIG_TYPE_GROUP);
        if (!item) {
            continue;
        }
        config_setting_set_string_simple(item, "mac", server->mac);
        config_setting_set_string_simple(item, "hostname", server->hostname);
        config_setting_set_string_simple(item, "address", server->serverInfo.address);
        if (!selected_set && cur->selected) {
            config_setting_set_bool_simple(item, "selected", true);
            selected_set = true;
        }
    }
    char *confdir = path_pref(), *conffile = path_join(confdir, CONF_NAME_HOSTS);
    free(confdir);
    config_write_file(&config, conffile);
    cleanup:
    config_destroy(&config);
    free(conffile);
}

PSERVER_DATA serverdata_new() {
    PSERVER_DATA server = malloc(sizeof(SERVER_DATA));
    SDL_memset(server, 0, sizeof(SERVER_DATA));
    return server;
}

PPCMANAGER_RESP serverinfo_resp_new() {
    PPCMANAGER_RESP resp = malloc(sizeof(PCMANAGER_RESP));
    SDL_memset(resp, 0, sizeof(PCMANAGER_RESP));
    return resp;
}

void serverinfo_resp_free(PPCMANAGER_RESP resp) {
    if (!resp->server_referenced && resp->server) {
        free((void *) resp->server);
    }
    free(resp);
}

void serverstate_setgserror(SERVER_STATE *state, int code, const char *msg) {
    state->code = SERVER_STATE_ERROR;
    state->error.errcode = code;
    state->error.errmsg = msg;
}

void pcmanager_resp_setgserror(PPCMANAGER_RESP resp, int code, const char *msg) {
    resp->result.code = code;
    resp->result.error.message = msg;
}

static inline void free_nullable(void *p) {
    if (!p) return;
    free(p);
}

void serverdata_free(PSERVER_DATA data) {
    free_nullable(data->modes);
    free_nullable((void *) data->uuid);
    free_nullable((void *) data->mac);
    free_nullable((void *) data->hostname);
    free_nullable((void *) data->gpuType);
    free_nullable((void *) data->gsVersion);
    free_nullable((void *) data->serverInfo.serverInfoAppVersion);
    free_nullable((void *) data->serverInfo.serverInfoGfeVersion);
    free_nullable((void *) data->serverInfo.address);
    free(data);
}

void serverlist_nodefree(PSERVER_LIST node) {
    if (node->apps) {
        applist_free(node->apps, applist_nodefree);
    }
    if (node->server) {
        serverdata_free((PSERVER_DATA) node->server);
    }
    free(node);
}

void pcmanager_request_update(const SERVER_DATA *server) {
    PSERVER_DATA arg = serverdata_new();
    SDL_memcpy(arg, server, sizeof(SERVER_DATA));
    SDL_CreateThread((SDL_ThreadFunction) _computer_manager_server_update_action, "srvupd", arg);
}

void pcmanager_register_callbacks(PPCMANAGER_CALLBACKS callbacks) {
    callbacks->next = NULL;
    callbacks->prev = NULL;
    callbacks_list = pcmanager_callbacks_append(callbacks_list, callbacks);
}

static int pcmanager_callbacks_comparator(PPCMANAGER_CALLBACKS p1, const void *p2) {
    return p1 != p2;
}

void pcmanager_unregister_callbacks(PPCMANAGER_CALLBACKS callbacks) {
    assert(callbacks);
    PPCMANAGER_CALLBACKS find = pcmanager_callbacks_find_by(callbacks_list, callbacks, pcmanager_callbacks_comparator);
    if (!find)
        return;
    callbacks_list = pcmanager_callbacks_remove(callbacks_list, find);
}

void serverlist_set_from_resp(PSERVER_LIST node, PPCMANAGER_RESP resp) {
    if (resp->state.code != SERVER_STATE_NONE) {
        SDL_memcpy(&node->state, &resp->state, sizeof(resp->state));
    }
    node->known = resp->known;
    node->server = resp->server;
    resp->server_referenced = true;
}

static void _pcmanager_client_setup_cb(void *data) {
    pcmanager_setup_running = data != NULL;
    if (data == NULL) {
        computer_manager_run_scan();
    }
}

static int _pcmanager_client_setup_action(void *unused) {
    (void) unused;
    bus_pushaction(_pcmanager_client_setup_cb, (void *) 1);
    app_gs_client_obtain();
    bus_pushaction(_pcmanager_client_setup_cb, (void *) 0);
    return 0;
}

void pcmanager_client_setup() {
    SDL_CreateThread(_pcmanager_client_setup_action, "clntinit", NULL);
}