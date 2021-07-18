#define PCMANAGER_IMPL
#include "computer_manager.h"
#include "pcmanager/priv.h"

#include "app.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <pthread.h>
#include <signal.h>

#include <libconfig.h>

#include "libgamestream/errors.h"
#include "error_manager.h"

#include "util/bus.h"
#include "util/user_event.h"
#include "util/path.h"

#include "stream/session.h"
#include "stream/settings.h"
#include "app.h"

#include "util/memlog.h"
#include "util/libconfig_ext.h"

static void strlower(char *p)
{
    for (; *p; ++p)
        *p = tolower(*p);
}

static pthread_t computer_manager_polling_thread;

PSERVER_LIST computer_list;
PPCMANAGER_CALLBACKS callbacks_list;

static void *_pcmanager_quitapp_action(void *data);
static void *_computer_manager_server_update_action(PSERVER_DATA data);
static void pcmanager_load_known_hosts();
static void pcmanager_save_known_hosts();
static void pcmanager_client_setup();

static void serverlist_set_from_resp(PSERVER_LIST node, PPCMANAGER_RESP resp);

bool pcmanager_setup_running = false;

void computer_manager_init()
{
    computer_list = NULL;
    pcmanager_load_known_hosts();
    pcmanager_client_setup();
    computer_manager_auto_discovery_start();
}

void computer_manager_destroy()
{
    computer_manager_auto_discovery_stop();
    if (computer_discovery_running)
    {
        pthread_kill(computer_manager_polling_thread, 0);
    }
    pcmanager_save_known_hosts();
    serverlist_free(computer_list, serverlist_nodefree);
}

bool computer_manager_dispatch_userevent(int which, void *data1, void *data2)
{
    return false;
}

void handle_server_updated(PPCMANAGER_RESP update)
{
    assert(update);
    if (update->result.code != GS_OK || !update->server)
        return;
    PSERVER_LIST node = serverlist_find_by(computer_list, update->server->uuid, serverlist_compare_uuid);
    if (!node)
        return;
    if (update->server_shallow)
        free((PSERVER_DATA)node->server);
    else
        serverdata_free((PSERVER_DATA)node->server);

    serverlist_set_from_resp(node, update);
}

void handle_server_discovered(PPCMANAGER_RESP discovered)
{
    if (discovered->state.code != SERVER_STATE_ONLINE)
        return;
    PSERVER_LIST node = serverlist_find_by(computer_list, discovered->server->uuid, serverlist_compare_uuid);
    if (node)
    {
        if (node->server)
        {
            serverdata_free((PSERVER_DATA)node->server);
        }
        serverlist_set_from_resp(node, discovered);
        for (PPCMANAGER_CALLBACKS cur = callbacks_list; cur != NULL; cur = cur->next)
            cur->onUpdated(discovered);
    }
    else
    {
        node = serverlist_new();
        serverlist_set_from_resp(node, discovered);

        computer_list = serverlist_append(computer_list, node);
        for (PPCMANAGER_CALLBACKS cur = callbacks_list; cur != NULL; cur = cur->next)
            cur->onAdded(discovered);
    }
}

bool computer_manager_run_scan()
{
    if (computer_discovery_running || streaming_status != STREAMING_NONE)
    {
        return false;
    }
    pthread_create(&computer_manager_polling_thread, NULL, _computer_manager_polling_action, NULL);
    return true;
}

static int server_list_namecmp(PSERVER_LIST item, const void *address)
{
    return strcmp(item->server->serverInfo.address, address);
}

bool pcmanager_quitapp(const SERVER_DATA *server, void (*callback)(PPCMANAGER_RESP))
{
    if (server->currentGame == 0)
    {
        return false;
    }
    cm_pin_request *req = malloc(sizeof(cm_pin_request));
    req->server = server;
    req->callback = callback;
    pthread_t quitapp_thread;
    pthread_create(&quitapp_thread, NULL, _pcmanager_quitapp_action, req);
    return true;
}

void *_pcmanager_quitapp_action(void *data)
{
    cm_pin_request *req = (cm_pin_request *)data;
    PPCMANAGER_RESP resp = serverinfo_resp_new();
    PSERVER_DATA server = serverdata_new();
    memcpy(server, req->server, sizeof(SERVER_DATA));
    int ret = gs_quit_app(app_gs_client_obtain(), server);
    if (ret != GS_OK)
        pcmanager_resp_setgserror(resp, ret, gs_error);
    resp->server = server;
    resp->server_shallow = true;
    bus_pushaction((bus_actionfunc)handle_server_updated, resp);
    bus_pushaction((bus_actionfunc)req->callback, resp);
    bus_pushaction((bus_actionfunc)serverinfo_resp_free, resp);
    free(req);
    return NULL;
}

void *_computer_manager_server_update_action(PSERVER_DATA data)
{
    PSERVER_DATA server = serverdata_new();
    PPCMANAGER_RESP update = serverinfo_resp_new();
    int ret = gs_init(app_gs_client_obtain(), server, strdup(data->serverInfo.address), app_configuration->unsupported);
    if (ret == GS_OK)
    {
        update->state.code = SERVER_STATE_ONLINE;
        update->server = server;
        update->server_shallow = false;
    }
    else
        serverstate_setgserror(&update->state, ret, gs_error);
    bus_pushaction((bus_actionfunc)handle_server_updated, update);
    bus_pushaction((bus_actionfunc)serverinfo_resp_free, update);
    free(data);
    return NULL;
}

int serverlist_compare_uuid(PSERVER_LIST other, const void *v)
{
    return strcmp(v, other->server->uuid);
}

void pcmanager_load_known_hosts()
{
    char *confdir = path_pref(), *conffile = path_join(confdir, CONF_NAME_HOSTS);
    free(confdir);
    config_t config;
    config_init(&config);
    int options = config_get_options(&config);
    options &= ~CONFIG_OPTION_OPEN_BRACE_ON_SEPARATE_LINE;
    options &= ~CONFIG_OPTION_COLON_ASSIGNMENT_FOR_GROUPS;
    config_set_options(&config, options);
    if (config_read_file(&config, conffile) != CONFIG_TRUE)
    {
        goto cleanup;
    }
    config_setting_t *root = config_root_setting(&config);
    for (int i = 0, j = config_setting_length(root); i < j; i++)
    {
        config_setting_t *item = config_setting_get_elem(root, i);
        const char *mac = config_setting_get_string_simple(item, "mac"),
                   *hostname = config_setting_get_string_simple(item, "hostname"),
                   *address = config_setting_get_string_simple(item, "address");
        if (!mac || !hostname || !address)
        {
            continue;
        }
        char *uuid = strdup(config_setting_name(item));
        strlower(uuid);
        PSERVER_DATA server = serverdata_new();
        server->uuid = uuid;
        server->mac = strdup(mac);
        server->hostname = strdup(hostname);
        server->serverInfo.address = strdup(address);

        PSERVER_LIST node = serverlist_new();
        node->state.code = SERVER_STATE_OFFLINE;
        node->server = server;
        node->known = true;
        computer_list = serverlist_append(computer_list, node);
        if (config_setting_get_bool_simple(item, "selected"))
        {
            app_configuration->address = strdup(address);
        }
    }
cleanup:
    config_destroy(&config);
    free(conffile);
}

void pcmanager_save_known_hosts()
{
    config_t config;
    config_init(&config);
    int options = config_get_options(&config);
    options &= ~CONFIG_OPTION_OPEN_BRACE_ON_SEPARATE_LINE;
    options &= ~CONFIG_OPTION_COLON_ASSIGNMENT_FOR_GROUPS;
    config_set_options(&config, options);
    config_setting_t *root = config_root_setting(&config);
    for (PSERVER_LIST cur = computer_list; cur != NULL; cur = cur->next)
    {
        if (!cur->server || !cur->known)
        {
            continue;
        }
        const SERVER_DATA *server = cur->server;
        config_setting_t *item = config_setting_add(root, server->uuid, CONFIG_TYPE_GROUP);
        config_setting_set_string_simple(item, "mac", server->mac);
        config_setting_set_string_simple(item, "hostname", server->hostname);
        config_setting_set_string_simple(item, "address", server->serverInfo.address);
        if (app_configuration->address && strcmp(app_configuration->address, server->serverInfo.address) == 0)
        {
            config_setting_set_bool_simple(item, "selected", true);
        }
    }
    char *confdir = path_pref(), *conffile = path_join(confdir, CONF_NAME_HOSTS);
    free(confdir);
    config_write_file(&config, conffile);
cleanup:
    config_destroy(&config);
    free(conffile);
}

PSERVER_DATA serverdata_new()
{
    PSERVER_DATA server = malloc(sizeof(SERVER_DATA));
    memset(server, 0, sizeof(SERVER_DATA));
    return server;
}

PPCMANAGER_RESP serverinfo_resp_new()
{
    PPCMANAGER_RESP resp = malloc(sizeof(PCMANAGER_RESP));
    memset(resp, 0, sizeof(PCMANAGER_RESP));
    return resp;
}

void serverinfo_resp_free(PPCMANAGER_RESP resp)
{
    if (!resp->server_referenced && resp->server)
    {
        free((void *)resp->server);
    }
    free(resp);
}

void serverstate_setgserror(SERVER_STATE *state, int code, const char *msg)
{
    state->code = SERVER_STATE_ERROR;
    state->error.errcode = code;
    state->error.errmsg = msg;
}

void pcmanager_resp_setgserror(PPCMANAGER_RESP resp, int code, const char *msg)
{
    resp->result.code = code;
    resp->result.error.message = msg;
}

#define free_nullable(p) \
    if (p)               \
    free((void *)p)

void serverdata_free(PSERVER_DATA data)
{
    free_nullable(data->modes);
    free_nullable(data->uuid);
    free_nullable(data->mac);
    free_nullable(data->hostname);
    free_nullable(data->gpuType);
    free_nullable(data->gsVersion);
    free_nullable(data->serverInfo.serverInfoAppVersion);
    free_nullable(data->serverInfo.serverInfoGfeVersion);
    free_nullable(data->serverInfo.address);
    free(data);
}

void serverlist_nodefree(PSERVER_LIST node)
{
    if (node->apps)
    {
        applist_free(node->apps, applist_nodefree);
    }
    if (node->server)
    {
        serverdata_free((PSERVER_DATA)node->server);
    }
    free(node);
}

void pcmanager_request_update(const SERVER_DATA *server)
{
    pthread_t update_thread;
    PSERVER_DATA arg = serverdata_new();
    memcpy(arg, server, sizeof(SERVER_DATA));
    pthread_create(&update_thread, NULL, (void *(*)(void *))_computer_manager_server_update_action, arg);
}

void pcmanager_register_callbacks(PPCMANAGER_CALLBACKS callbacks)
{
    callbacks_list = pcmanager_callbacks_append(callbacks_list, callbacks);
}

static int pcmanager_callbacks_comparator(PPCMANAGER_CALLBACKS p1, const void *p2)
{
    return p1 != p2;
}

void pcmanager_unregister_callbacks(PPCMANAGER_CALLBACKS callbacks)
{
    assert(callbacks);
    PPCMANAGER_CALLBACKS find = pcmanager_callbacks_find_by(callbacks_list, callbacks, pcmanager_callbacks_comparator);
    if (!find)
        return;
    callbacks_list = pcmanager_callbacks_remove(callbacks_list, find);
}

void serverlist_set_from_resp(PSERVER_LIST node, PPCMANAGER_RESP resp)
{
    if (resp->state.code != SERVER_STATE_NONE)
    {
        memcpy(&node->state, &resp->state, sizeof(resp->state));
    }
    node->known = resp->known;
    node->server = resp->server;
    resp->server_referenced = true;
}

static void _pcmanager_client_setup_cb(void *data)
{
    pcmanager_setup_running = data != NULL;
    if (data == NULL)
    {
        computer_manager_run_scan();
    }
}

static void *_pcmanager_client_setup_action(void *unused)
{
    bus_pushaction(_pcmanager_client_setup_cb, (void *)1);
    app_gs_client_obtain();
    bus_pushaction(_pcmanager_client_setup_cb, (void *)0);
    return NULL;
}

void pcmanager_client_setup()
{
    pthread_t setup_thread;
    pthread_create(&setup_thread, NULL, _pcmanager_client_setup_action, NULL);
}