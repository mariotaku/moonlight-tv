#include <glib.h>
#include <SDL.h>

#include "sdl/user_event.h"
#include "application_manager.h"
#include "computer_manager.h"

#include "libgamestream/client.h"
#include "libgamestream/errors.h"

static GHashTable *computer_applications = NULL;

static gpointer _application_manager_applist_action(gpointer data);
static bool _application_manager_applist_result(const char *address, PAPP_LIST list);

void application_manager_init()
{
    computer_applications = g_hash_table_new(g_str_hash, g_str_equal);
}

void application_manager_destroy()
{
    g_hash_table_unref(computer_applications);
}

void application_manager_load(SERVER_DATA *server)
{
    g_thread_new("am_applist", _application_manager_applist_action, server);
}

bool application_manager_dispatch_event(SDL_Event ev)
{
    if (ev.user.code == SDL_USER_AM_APPLIST_ARRIVED)
    {
        _application_manager_applist_result((const char *)ev.user.data1, (PAPP_LIST)ev.user.data2);
        return true;
    }
    return false;
}

PAPP_LIST application_manager_list_of(const char *address)
{
    return g_hash_table_lookup(computer_applications, address);
}

int applist_len(PAPP_LIST p)
{
    int length = 0;
    PAPP_LIST cur = p;
    while ((cur = cur->next) != NULL)
    {
        length++;
    }
    return length;
}

PAPP_LIST applist_nth(PAPP_LIST p, int n)
{
    PAPP_LIST ret = NULL;
    int i = 0;
    for (ret = p; ret != NULL && i < n; ret = ret->next, i++)
        ;
    return ret;
}

bool _application_manager_applist_result(const char *address, PAPP_LIST list)
{
    g_hash_table_insert(computer_applications, (void *)address, list);
    return true;
}

gpointer _application_manager_applist_action(gpointer data)
{
    SERVER_DATA *server = (SERVER_DATA *)data;
    PAPP_LIST list = NULL;
    if (gs_applist(server, &list) != GS_OK)
    {
        g_warning("Can't get app list\n", NULL);
        return NULL;
    }

    SDL_Event ev;
    ev.type = SDL_USEREVENT;
    ev.user.code = SDL_USER_AM_APPLIST_ARRIVED;
    ev.user.data1 = (void *)server->serverInfo.address;
    ev.user.data2 = list;
    SDL_PushEvent(&ev);
    return NULL;
}