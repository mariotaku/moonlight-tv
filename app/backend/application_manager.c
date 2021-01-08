#include <pthread.h>

#include "util/user_event.h"
#include "application_manager.h"
#include "computer_manager.h"
#include "util/bus.h"

#include "libgamestream/client.h"
#include "libgamestream/errors.h"

static pthread_t _application_manager_load_thread;

static void *_application_manager_applist_action(void *data);
static bool _application_manager_applist_result(PSERVER_LIST node, PAPP_LIST list);

void application_manager_init()
{
}

void application_manager_destroy()
{
}

void application_manager_load(PSERVER_LIST node)
{
    pthread_create(&_application_manager_load_thread, NULL, _application_manager_applist_action, node);
}

bool application_manager_dispatch_userevent(int which, void *data1, void *data2)
{
    if (which == USER_AM_APPLIST_ARRIVED)
    {
        _application_manager_applist_result((PSERVER_LIST)data1, (PAPP_LIST)data2);
        return true;
    }
    return false;
}

PAPP_LIST application_manager_list_of(const char *address)
{
    PSERVER_LIST node = computer_manager_server_of(address);
    if (node == NULL)
    {
        return NULL;
    }
    return node->apps;
}

bool _application_manager_applist_result(PSERVER_LIST node, PAPP_LIST list)
{
    node->apps = list;
    return true;
}

void *_application_manager_applist_action(void *data)
{
    PSERVER_LIST node = (PSERVER_LIST)data;
    PAPP_LIST list = NULL;
    if (gs_applist(node->server, &list) != GS_OK)
    {
        fprintf(stderr, "Can't get app list\n");
        pthread_exit(NULL);
        return NULL;
    }

    bus_pushevent(USER_AM_APPLIST_ARRIVED, data, list);
    pthread_exit(NULL);
    return NULL;
}