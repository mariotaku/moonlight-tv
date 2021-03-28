#include "backend/computer_manager.h"
#include "priv.h"

#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <signal.h>

#include "libgamestream/errors.h"

#include "util/bus.h"
#include "util/user_event.h"

#include "util/memlog.h"

static void *_computer_manager_pairing_action(void *data);
static void *_computer_manager_unpairing_action(void *data);
static void *_manual_adding_action(void *data);
static int pin_random(int min, int max);

bool computer_manager_pair(PSERVER_LIST node, char *pin, void (*callback)(PSERVER_LIST))
{
    int pin_int = pin_random(0, 9999);
    cm_pin_request *req = malloc(sizeof(cm_pin_request));
    snprintf(pin, 5, "%04u", pin_int);
    req->pin = strdup(pin);
    req->node = node;
    req->callback = callback;
    pthread_t pair_thread;
    pthread_create(&pair_thread, NULL, _computer_manager_pairing_action, req);
    return true;
}

bool computer_manager_unpair(PSERVER_LIST node, void (*callback)(PSERVER_LIST))
{
    cm_pin_request *req = malloc(sizeof(cm_pin_request));
    req->node = node;
    req->callback = callback;
    pthread_t pair_thread;
    pthread_create(&pair_thread, NULL, _computer_manager_unpairing_action, req);
    return true;
}

bool pcmanager_manual_add(const char *address)
{
    pthread_t add_thread;
    pthread_create(&add_thread, NULL, _manual_adding_action, (void *)address);
    return true;
}

void *_computer_manager_pairing_action(void *data)
{
    cm_pin_request *req = (cm_pin_request *)data;
    PSERVER_LIST node = req->node;
    // Pairing will change server pointer
    PSERVER_DATA server = (PSERVER_DATA)node->server;
    int ret = gs_pair(server, (char *)req->pin);
    if (ret != GS_OK)
        serverstate_setgserror(&node->state, ret, gs_error);
    bus_pushaction((bus_actionfunc)req->callback, node);
    free(req);
    return NULL;
}

void *_computer_manager_unpairing_action(void *data)
{
    cm_pin_request *req = (cm_pin_request *)data;
    PSERVER_LIST node = req->node;
    // Pairing will change server pointer
    PSERVER_DATA server = (PSERVER_DATA)node->server;
    int ret = gs_unpair(server);
    if (ret != GS_OK)
        serverstate_setgserror(&node->state, ret, gs_error);
    bus_pushaction((bus_actionfunc)req->callback, node);
    free(req);
    return NULL;
}

void *_manual_adding_action(void *data)
{
    char *address = data;
    pcmanager_insert_by_address(address, true);
    return NULL;
}

static int pin_random(int min, int max)
{
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}
