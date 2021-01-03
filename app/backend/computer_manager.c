#include "computer_manager.h"

#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "libgamestream/errors.h"
#include "error_manager.h"

#define LINKEDLIST_TYPE PSERVER_LIST
#include "util/linked_list.h"

static SDL_Thread *computer_manager_polling_thread = NULL;

typedef struct CM_PIN_REQUEST_T
{
    PSERVER_DATA server;
    char *pin;
    pairing_callback callback;
} cm_pin_request;

static int _computer_manager_pairing_action(void *data);
static PSERVER_LIST computer_list;

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

bool computer_manager_polling_start()
{
    if (computer_manager_polling_thread)
    {
        return false;
    }
    computer_manager_polling_thread = SDL_CreateThread(_computer_manager_polling_action, "cm_polling", NULL);
    return true;
}

void computer_manager_polling_stop()
{
    if (!computer_manager_polling_thread)
    {
        return;
    }
    computer_manager_polling_thread = NULL;
}

PSERVER_LIST computer_manager_list()
{
    return computer_list;
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

bool computer_manager_pair(SERVER_DATA *p, char *pin, pairing_callback cb)
{
    int pin_int = pin_random(0, 9999);
    cm_pin_request *req = malloc(sizeof(cm_pin_request));
    snprintf(pin, 5, "%04u", pin_int);
    req->pin = strdup(pin);
    req->server = p;
    req->callback = cb;
    SDL_CreateThread(_computer_manager_pairing_action, "cm_pairing", req);
    return true;
}

void _computer_manager_add(SERVER_DATA *p)
{
    PSERVER_LIST node = linkedlist_new(PSERVER_LIST);
    node->server = p;
    node->apps = NULL;
    if (computer_list == NULL)
    {
        computer_list = node;
    }
    else
    {
        computer_list = linkedlist_append(computer_list, node);
    }
}

int _computer_manager_pairing_action(void *data)
{
    cm_pin_request *req = (cm_pin_request *)data;
    PSERVER_DATA server = req->server;
    int result = gs_pair(req->server, (char *)req->pin);
    req->callback(result, gs_error);
    free(data);
    return 0;
}