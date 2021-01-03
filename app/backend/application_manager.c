#include <SDL.h>

#include "sdl/user_event.h"
#include "application_manager.h"
#include "computer_manager.h"

#include "libgamestream/client.h"
#include "libgamestream/errors.h"

static int _application_manager_applist_action(void *data);
static bool _application_manager_applist_result(PSERVER_LIST node, PAPP_LIST list);

void application_manager_init()
{
}

void application_manager_destroy()
{
}

void application_manager_load(PSERVER_LIST node)
{
    SDL_CreateThread(_application_manager_applist_action, "am_applist", node);
}

bool application_manager_dispatch_userevent(SDL_Event ev)
{
    if (ev.user.code == SDL_USER_AM_APPLIST_ARRIVED)
    {
        _application_manager_applist_result((PSERVER_LIST)ev.user.data1, (PAPP_LIST)ev.user.data2);
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

int _application_manager_applist_action(void *data)
{
    PSERVER_LIST node = (PSERVER_LIST)data;
    PAPP_LIST list = NULL;
    if (gs_applist(node->server, &list) != GS_OK)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Can't get app list\n");
        return 0;
    }

    SDL_Event ev;
    ev.type = SDL_USEREVENT;
    ev.user.code = SDL_USER_AM_APPLIST_ARRIVED;
    ev.user.data1 = data;
    ev.user.data2 = list;
    SDL_PushEvent(&ev);
    return 0;
}