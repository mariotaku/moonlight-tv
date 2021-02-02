#include "computer_manager.h"

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

static pthread_t computer_manager_polling_thread;
bool computer_manager_executing_quitapp;
typedef struct CM_PIN_REQUEST_T
{
    PSERVER_LIST node;
    char *pin;
    pairing_callback callback;
} cm_pin_request;

PSERVER_LIST computer_list;

static void *_computer_manager_pairing_action(void *data);
static void *_computer_manager_quitapp_action(void *data);
static void *_computer_manager_server_update_action(void *data);
static int _server_list_compare_uuid(PSERVER_LIST other, const void *v);
static void pcmanager_load_known_hosts();
static void pcmanager_save_known_hosts();

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
    serverlist_free(computer_list);
}

bool computer_manager_dispatch_userevent(int which, void *data1, void *data2)
{
    switch (which)
    {
    case USER_CM_REQ_SERVER_UPDATE:
    {
        /* code */
        pthread_t update_thread;
        pthread_create(&update_thread, NULL, _computer_manager_server_update_action, data1);
        return true;
    }
    case USER_CM_REQ_SERVER_UPDATED:
    {
        PSERVER_LIST orig = data1, update = data2;
        orig->err = update->err;
        if (update->err == GS_OK)
        {
            PSERVER_DATA old = orig->server;
            free(old);
            orig->server = update->server;
        }
        else
        {
            orig->errmsg = update->errmsg;
            orig->server = NULL;
            free(update->server);
        }
        free(update);
        return true;
    }
    case USER_CM_SERVER_DISCOVERED:
    {
        PSERVER_LIST discovered = data1;

        PSERVER_LIST find = serverlist_find_by(computer_list, discovered->uuid, _server_list_compare_uuid);
        if (find)
        {
            PSERVER_DATA oldsrv = find->server;
            find->mac = discovered->mac;
            find->hostname = discovered->hostname;
            find->err = discovered->err;
            find->errmsg = discovered->errmsg;
            find->server = discovered->server;
            if (oldsrv)
            {
                free(oldsrv);
            }
            bus_pushevent(USER_CM_SERVER_UPDATED, find, NULL);
        }
        else
        {
            computer_list = serverlist_append(computer_list, discovered);
            bus_pushevent(USER_CM_SERVER_ADDED, discovered, NULL);
        }
        return true;
    }
    default:
        break;
    }
    return false;
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

PSERVER_LIST computer_manager_server_of(const char *address)
{
    return serverlist_find_by(computer_list, address, server_list_namecmp);
}

PSERVER_LIST computer_manager_server_at(int index)
{
    return serverlist_nth(computer_list, index);
}

static int pin_random(int min, int max)
{
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

bool computer_manager_pair(PSERVER_LIST node, char *pin, pairing_callback cb)
{
    int pin_int = pin_random(0, 9999);
    cm_pin_request *req = malloc(sizeof(cm_pin_request));
    snprintf(pin, 5, "%04u", pin_int);
    req->pin = strdup(pin);
    req->node = node;
    req->callback = cb;
    pthread_t pair_thread;
    pthread_create(&pair_thread, NULL, _computer_manager_pairing_action, req);
    return true;
}

bool computer_manager_quitapp(PSERVER_LIST node)
{
    if (node->server->currentGame == 0)
    {
        return false;
    }
    pthread_t quitapp_thread;
    pthread_create(&quitapp_thread, NULL, _computer_manager_quitapp_action, node);
    return true;
}

void *_computer_manager_pairing_action(void *data)
{
    cm_pin_request *req = (cm_pin_request *)data;
    PSERVER_LIST node = req->node;
    PSERVER_DATA server = node->server;
    int result = gs_pair(server, (char *)req->pin);
    req->callback(node, result, gs_error);
    free(data);
    return NULL;
}

void *_computer_manager_quitapp_action(void *data)
{
    computer_manager_executing_quitapp = true;
    PSERVER_LIST node = data;
    int quitret = gs_quit_app(node->server);
    computer_manager_executing_quitapp = false;
    bus_pushevent(USER_CM_REQ_SERVER_UPDATE, node, NULL);
    if (quitret != GS_OK)
    {
        bus_pushevent(USER_CM_QUITAPP_FAILED, node, NULL);
    }
    return NULL;
}

void *_computer_manager_server_update_action(void *data)
{
    PSERVER_LIST node = data;
    PSERVER_LIST update = malloc(sizeof(SERVER_LIST));
    memset(update, 0, sizeof(SERVER_LIST));
    update->server = malloc(sizeof(SERVER_DATA));
    update->err = gs_init(update->server, (char *)node->server->serverInfo.address, app_configuration->key_dir,
                          app_configuration->debug_level, app_configuration->unsupported);
    if (update->err)
    {
        update->errmsg = gs_error;
    }
    bus_pushevent(USER_CM_REQ_SERVER_UPDATED, node, update);
    return NULL;
}

int _server_list_compare_uuid(PSERVER_LIST other, const void *v)
{
    return strcmp(v, other->uuid);
}

void pcmanager_load_known_hosts()
{
    char *confdir = settings_config_dir(), *conffile = path_join(confdir, "known_hosts");
    FILE *fd = fopen(conffile, "r");
    free(conffile);
    if (fd == NULL)
    {
        return;
    }

    char *line = NULL;
    size_t len = 0;

    while (getline(&line, &len, fd) != -1)
    {
        if (strlen(line) > 100)
        {
            continue;
        }
        char uuid[40], mac[20], hostname[40];
        if (sscanf(line, "%s %s %s", uuid, mac, hostname) != 3)
        {
            continue;
        }
        PSERVER_LIST node = serverlist_new();
        node->uuid = strdup(uuid);
        node->mac = strdup(mac);
        node->hostname = strdup(hostname);
        node->known = true;
        computer_list = serverlist_append(computer_list, node);
    }
    fclose(fd);
}

void pcmanager_save_known_hosts()
{
    char *confdir = settings_config_dir(), *conffile = path_join(confdir, "known_hosts");
    FILE *fd = fopen(conffile, "w");
    free(conffile);
    if (fd == NULL)
    {
        return;
    }
    SERVER_LIST *cur = computer_list;
    while (cur != NULL)
    {
        if (!cur->known || !cur->uuid || !cur->mac || !cur->hostname)
        {
            continue;
        }
        fprintf(fd, "%s %s %s\n", cur->uuid, cur->mac, cur->hostname);
        cur = cur->next;
    }
    fclose(fd);
}