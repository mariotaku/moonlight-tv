#include "computer_manager.h"

#include <stdlib.h>
#include <glib.h>

#include "libgamestream/errors.h"
#include "error_manager.h"

static GThread *computer_manager_polling_thread = NULL;

static struct COMPUTER_MANAGER_T
{
    GList *computer_list;
} computer_manager;

typedef struct CM_PIN_REQUEST_T
{
    PSERVER_DATA server;
    char *pin;
    pairing_callback callback;
} cm_pin_request;

static gpointer _computer_manager_pairing_action(gpointer data);

void computer_manager_init()
{
    (&computer_manager)->computer_list = NULL;
    computer_manager_polling_start();
}

void computer_manager_destroy()
{
    computer_manager_polling_stop();
    
    g_list_free((&computer_manager)->computer_list);
}

bool computer_manager_polling_start()
{
    if (computer_manager_polling_thread)
    {
        return false;
    }
    computer_manager_polling_thread = g_thread_new("cm_polling", _computer_manager_polling_action, NULL);
    return true;
}

void computer_manager_polling_stop()
{
    if (!computer_manager_polling_thread)
    {
        return;
    }
    g_thread_unref(computer_manager_polling_thread);
    computer_manager_polling_thread = NULL;
}

GList *computer_manager_list()
{
    return (&computer_manager)->computer_list;
}

SERVER_DATA *computer_manager_server_at(int index)
{
    return g_list_nth_data((&computer_manager)->computer_list, index);
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
    g_thread_new("cm_pairing", _computer_manager_pairing_action, req);
    return true;
}

void _computer_manager_add(SERVER_DATA *p)
{
    struct COMPUTER_MANAGER_T *cm = &computer_manager;
    cm->computer_list = g_list_append(cm->computer_list, p);
}

gpointer _computer_manager_pairing_action(gpointer data)
{
    cm_pin_request *req = (cm_pin_request *)data;
    PSERVER_DATA server = req->server;
    int result = gs_pair(req->server, (char *)req->pin);
    req->callback(result, gs_error);
    g_free(data);
    return NULL;
}