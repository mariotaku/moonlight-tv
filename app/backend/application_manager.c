#include <pthread.h>

#include "util/user_event.h"
#include "application_manager.h"
#include "computer_manager.h"
#include "util/bus.h"

#include "libgamestream/client.h"
#include "libgamestream/errors.h"

#define LINKEDLIST_TYPE APP_DLIST
#define LINKEDLIST_DOUBLE 1
#include "util/linked_list.h"

static pthread_t _application_manager_load_thread;

static void *_application_manager_applist_action(void *data);
static bool _application_manager_applist_result(PSERVER_LIST node, PAPP_DLIST list);

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
        _application_manager_applist_result((PSERVER_LIST)data1, (PAPP_DLIST)data2);
        return true;
    }
    return false;
}

PAPP_DLIST application_manager_list_of(const char *address)
{
    PSERVER_LIST node = computer_manager_server_of(address);
    if (node == NULL)
    {
        return NULL;
    }
    return node->apps;
}

bool _application_manager_applist_result(PSERVER_LIST node, PAPP_DLIST list)
{
    node->apps = list;
    return true;
}

static int _applist_name_comparator(PAPP_DLIST p1, PAPP_DLIST p2)
{
    return strcmp(p1->name, p2->name);
}

void *_application_manager_applist_action(void *data)
{
    PSERVER_LIST node = (PSERVER_LIST)data;
    PAPP_LIST list = NULL, tmp;
    if (gs_applist(node->server, &list) != GS_OK)
    {
        fprintf(stderr, "Can't get app list\n");
        pthread_exit(NULL);
        return NULL;
    }
    PAPP_DLIST dlist = NULL;
    for (PAPP_LIST cur = list; cur != NULL; cur = cur->next)
    {
        PAPP_DLIST item = linkedlist_new();
        item->id = cur->id;
        item->name = cur->name;
        dlist = linkedlist_sortedinsert(dlist, item, _applist_name_comparator);
    }
    // Free the single linked list
    while (list != NULL)
    {
        tmp = list;
        list = list->next;
        free(tmp);
    }

    bus_pushevent(USER_AM_APPLIST_ARRIVED, data, dlist);
    pthread_exit(NULL);
    return NULL;
}