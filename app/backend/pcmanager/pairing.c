#include "backend/computer_manager.h"
#include "priv.h"

#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <signal.h>

#include "libgamestream/errors.h"

#include "util/bus.h"
#include "util/user_event.h"

static void *_computer_manager_pairing_action(void *data);
static void *_manual_adding_action(void *data);
static int pin_random(int min, int max);

bool computer_manager_pair(PSERVER_LIST node, char *pin)
{
    int pin_int = pin_random(0, 9999);
    cm_pin_request *req = malloc(sizeof(cm_pin_request));
    snprintf(pin, 5, "%04u", pin_int);
    req->pin = strdup(pin);
    req->node = node;
    pthread_t pair_thread;
    pthread_create(&pair_thread, NULL, _computer_manager_pairing_action, req);
    return true;
}

bool pcmanager_manual_add(char *address)
{
    pthread_t add_thread;
    pthread_create(&add_thread, NULL, _manual_adding_action, address);
    return true;
}

void *_computer_manager_pairing_action(void *data)
{
    cm_pin_request *req = (cm_pin_request *)data;
    PSERVER_LIST node = req->node;
    // Pairing will change server pointer
    PSERVER_DATA server = (PSERVER_DATA) node->server;
    node->err = gs_pair(server, (char *)req->pin);
    node->errmsg = gs_error;
    bus_pushevent(USER_CM_PAIRING_DONE, node, NULL);
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
