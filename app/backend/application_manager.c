#define APPMANAGER_IMPL
#include "app.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <pthread.h>

#include "util/user_event.h"
#include "application_manager.h"
#include "util/bus.h"
#include "util/logging.h"

#include "libgamestream/client.h"
#include "libgamestream/errors.h"

static pthread_t _application_manager_load_thread;
static PAPPMANAGER_CALLBACKS callbacks_list;

static void *_application_manager_applist_action(PSERVER_LIST node);
static bool _application_manager_applist_result(PSERVER_LIST node, PAPP_DLIST list);

void application_manager_init()
{
}

void application_manager_destroy()
{
}

void application_manager_load(PSERVER_LIST node)
{
    if (node->appload)
    {
        return;
    }
    node->appload = true;
    pthread_create(&_application_manager_load_thread, NULL, (void *(*)(void *))_application_manager_applist_action, node);
}

bool application_manager_dispatch_userevent(int which, void *data1, void *data2)
{
    if (which == USER_AM_APPLIST_LOADED)
    {
        _application_manager_applist_result((PSERVER_LIST)data1, (PAPP_DLIST)data2);
        return true;
    }
    return false;
}

void appmanager_register_callbacks(PAPPMANAGER_CALLBACKS callbacks)
{
    callbacks_list = appmanager_callbacks_append(callbacks_list, callbacks);
}

static int appmanager_callbacks_comparator(PAPPMANAGER_CALLBACKS p1, const void *p2)
{
    return p1 != p2;
}

void appmanager_unregister_callbacks(PAPPMANAGER_CALLBACKS callbacks)
{
    assert(callbacks);
    PAPPMANAGER_CALLBACKS find = appmanager_callbacks_find_by(callbacks_list, callbacks, appmanager_callbacks_comparator);
    if (!find)
        return;
    callbacks_list = appmanager_callbacks_remove(callbacks_list, find);
}

bool _application_manager_applist_result(PSERVER_LIST node, PAPP_DLIST list)
{
    node->apps = list;
    node->applen = list ? applist_len(list) : 0;
    node->appload = false;
    for (PAPPMANAGER_CALLBACKS cur = callbacks_list; cur != NULL; cur = cur->next)
            cur->onAppsUpdated(node);
    return true;
}

static int _applist_name_comparator(PAPP_DLIST p1, PAPP_DLIST p2)
{
    return strcmp(p1->name, p2->name);
}

void *_application_manager_applist_action(PSERVER_LIST node)
{
#if _GNU_SOURCE
    pthread_setname_np(pthread_self(), "applist");
#endif
    PAPP_LIST list = NULL, tmp;
    int ret;
    if ((ret = gs_applist(app_gs_client_obtain(), (PSERVER_DATA)node->server, &list)) != GS_OK)
    {
        applog_w("AppMgr", "Can't get app list: %d", ret);
        bus_pushevent(USER_AM_APPLIST_LOADED, node, NULL);
        return NULL;
    }
    PAPP_DLIST dlist = NULL;
    for (PAPP_LIST cur = list; cur != NULL; cur = cur->next)
    {
        PAPP_DLIST item = applist_new();
        item->id = cur->id;
        item->name = cur->name;
        item->hdr = cur->hdr;
        dlist = applist_sortedinsert(dlist, item, _applist_name_comparator);
    }
    // Free the single linked list
    while (list != NULL)
    {
        tmp = list;
        list = list->next;
        free(tmp);
    }

    bus_pushevent(USER_AM_APPLIST_LOADED, node, dlist);
    return NULL;
}

void applist_nodefree(PAPP_DLIST node)
{
    if (node->name)
    {
        free(node->name);
    }
    free(node);
}