#include "computer_manager.h"
#include "pcmanager/priv.h"

#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <signal.h>

#include "libgamestream/errors.h"
#include "error_manager.h"

#include "util/bus.h"
#include "util/user_event.h"
#include "util/path.h"

#include "stream/session.h"
#include "stream/settings.h"
#include "app.h"

#include "util/memlog.h"

static pthread_t computer_manager_polling_thread;

PSERVER_LIST computer_list;

static void *_pcmanager_quitapp_action(void *data);
static void *_computer_manager_server_update_action(PSERVER_DATA data);
static void pcmanager_load_known_hosts();
static void pcmanager_save_known_hosts();

static void serverlist_set_from_resp(PSERVER_LIST node, PPCMANAGER_RESP resp);

void computer_manager_init()
{
    computer_list = NULL;
    pcmanager_load_known_hosts();
    computer_manager_run_scan();
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
    PSERVER_LIST node = serverlist_find_by(computer_list, discovered->server->uuid, serverlist_compare_uuid);
    if (node)
    {
        if (node->server)
        {
            serverdata_free((PSERVER_DATA)node->server);
        }
        serverlist_set_from_resp(node, discovered);
        bus_pushevent(USER_CM_SERVER_UPDATED, discovered, NULL);
    }
    else
    {
        node = serverlist_new();
        serverlist_set_from_resp(node, discovered);

        computer_list = serverlist_append(computer_list, node);
        bus_pushevent(USER_CM_SERVER_ADDED, discovered, NULL);
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
    int ret = gs_quit_app(server);
    if (ret != GS_OK)
        serverstate_setgserror(&resp->state, ret, gs_error);
    resp->server = server;
    bus_pushaction((bus_actionfunc)req->callback, resp);
    bus_pushaction((bus_actionfunc)handle_server_updated, resp);
    bus_pushaction((bus_actionfunc)serverinfo_resp_free, resp);
    free(req);
    return NULL;
}

void *_computer_manager_server_update_action(PSERVER_DATA data)
{
    PSERVER_DATA server = serverdata_new();
    PPCMANAGER_RESP update = serverinfo_resp_new();
    int ret = gs_init(server, strdup(data->serverInfo.address), app_configuration->key_dir,
                      app_configuration->debug_level, app_configuration->unsupported);
    update->server = server;
    update->server_shallow = false;
    if (ret == GS_OK)
        update->state.code = SERVER_STATE_ONLINE;
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
    char *confdir = path_pref(), *conffile = path_join(confdir, "known_hosts");
    FILE *fd = fopen(conffile, "r");
    free(confdir);
    free(conffile);
    if (fd == NULL)
    {
        return;
    }

    char *line = NULL;
    size_t len = 0;

    while (getline(&line, &len, fd) != -1)
    {
        if (strlen(line) >= 150)
        {
            continue;
        }
        // UUID: 37 including NUL
        // MAC: 17 including NUL
        // HOSTNAME: 17 including NUL
        // ADDRESS: 57 including NUL
        char uuid[150], mac[150], hostname[150], address[150];
        if (sscanf(line, "%s %s %s %s", uuid, mac, hostname, address) != 4)
        {
            continue;
        }
        PSERVER_DATA server = serverdata_new();
        server->uuid = strdup(uuid);
        server->mac = strdup(mac);
        server->hostname = strdup(hostname);
        server->serverInfo.address = strdup(address);

        PSERVER_LIST node = serverlist_new();
        node->state.code = SERVER_STATE_OFFLINE;
        node->server = server;
        node->known = true;
        computer_list = serverlist_append(computer_list, node);
    }
    fclose(fd);
}

void pcmanager_save_known_hosts()
{
    char *confdir = path_pref(), *conffile = path_join(confdir, "known_hosts");
    FILE *fd = fopen(conffile, "w");
    free(confdir);
    free(conffile);
    if (fd == NULL)
    {
        return;
    }
    for (PSERVER_LIST cur = computer_list; cur != NULL; cur = cur->next)
    {
        if (!cur->server || !cur->known)
        {
            continue;
        }
        const SERVER_DATA *server = cur->server;
        fprintf(fd, "%s %s %s %s\n", server->uuid, server->mac, server->hostname, server->serverInfo.address);
    }
    fclose(fd);
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
    if (!resp->server_referenced)
    {
        free(resp->server);
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

static void serverlist_set_from_resp(PSERVER_LIST node, PPCMANAGER_RESP resp)
{
    if (resp->state.code != SERVER_STATE_NONE)
    {
        memcpy(&node->state, &resp->state, sizeof(resp->state));
    }
    node->known = resp->known;
    node->server = resp->server;
    resp->server_referenced = true;
}