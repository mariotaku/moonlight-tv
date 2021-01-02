#include <glib.h>

#include "application_manager.h"
#include "computer_manager.h"

#include "libgamestream/client.h"
#include "libgamestream/errors.h"

static GHashTable *computer_applications = NULL;

static gpointer _application_manager_applist_action(gpointer data);

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

static gboolean _application_manager_applist_result(gpointer data)
{
    PAPP_LIST list = (PAPP_LIST)data;
    g_message("_application_manager_applist_result", NULL);
    return TRUE;
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

    for (int i = 1; list != NULL; i++)
    {
        g_message("%d. %s\n", i, list->name, NULL);
        list = list->next;
    }
    return NULL;
}