#include "computer_manager.h"

#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include "libgamestream/errors.h"
#include "error_manager.h"

#define LINKEDLIST_TYPE PSERVER_LIST
#include "util/linked_list.h"
#include "util/bus.h"
#include "util/user_event.h"

#include "stream/settings.h"

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

void computer_manager_init()
{
    computer_list = NULL;
    computer_manager_polling_start();
}

void computer_manager_destroy()
{
    computer_manager_polling_stop();

    linkedlist_free(computer_list);
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
    case USER_CM_SERVER_UPDATED:
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
    default:
        break;
    }
    return false;
}

bool computer_manager_polling_start()
{
    pthread_create(&computer_manager_polling_thread, NULL, _computer_manager_polling_action, NULL);
    return true;
}

void computer_manager_polling_stop()
{
}

static int server_list_namecmp(PSERVER_LIST item, const void *address)
{
    return strcmp(item->server->address, address);
}

PSERVER_LIST computer_manager_server_of(const char *address)
{
    return linkedlist_find_by(computer_list, address, server_list_namecmp);
}

PSERVER_LIST computer_manager_server_at(int index)
{
    return linkedlist_nth(computer_list, index);
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

void _computer_manager_add(char *name, PSERVER_DATA p, int err)
{
    PSERVER_LIST node = linkedlist_new(SERVER_LIST);
    node->name = name;
    node->server = p;
    node->err = err;
    node->errmsg = err != GS_OK ? gs_error : NULL;
    node->apps = NULL;
    computer_list = linkedlist_append(computer_list, node);
}

void *_computer_manager_pairing_action(void *data)
{
    cm_pin_request *req = (cm_pin_request *)data;
    PSERVER_LIST node = req->node;
    PSERVER_DATA server = node->server;
    int result = gs_pair(server, (char *)req->pin);
    req->callback(node, result, gs_error);
    free(data);
    pthread_exit(NULL);
    return NULL;
}

void *_computer_manager_quitapp_action(void *data)
{
    computer_manager_executing_quitapp = true;
    PSERVER_LIST node = data;
    gs_quit_app(node->server);
    computer_manager_executing_quitapp = false;
    bus_pushevent(USER_CM_REQ_SERVER_UPDATE, node, NULL);
    pthread_exit(NULL);
    return NULL;
}

void *_computer_manager_server_update_action(void *data)
{
    PSERVER_LIST node = data;
    PSERVER_LIST update = malloc(sizeof(SERVER_LIST));
    memset(update, 0, sizeof(SERVER_LIST));
    update->server = malloc(sizeof(SERVER_DATA));
    PCONFIGURATION config = settings_load();
    update->err = gs_init(update->server, (char *)node->server->serverInfo.address, config->key_dir,
                          config->debug_level, config->unsupported);
    if (update->err)
    {
        update->errmsg = gs_error;
    }
    bus_pushevent(USER_CM_SERVER_UPDATED, node, update);
    pthread_exit(NULL);
    return NULL;
}