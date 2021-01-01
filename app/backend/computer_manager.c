#include "computer_manager.h"

#include <stdlib.h>
#include <glib.h>

#include "error_manager.h"
#include "nvapp.h"

static GThread *computer_manager_polling_thread = NULL;

static struct COMPUTER_MANAGER_T
{
    GList *computer_list;
} computer_manager;

void computer_manager_init()
{
    (&computer_manager)->computer_list = NULL;
    computer_manager_polling_start();
}

void computer_manager_destroy()
{
    computer_manager_polling_stop();
    g_list_free_full((&computer_manager)->computer_list, (GDestroyNotify)nvcomputer_free);
}

void computer_manager_polling_start()
{
    if (computer_manager_polling_thread)
    {
        g_warning("computer_manager_polling_start() called when polling already started");
        return;
    }
    computer_manager_polling_thread = g_thread_new("cm_polling", _computer_manager_polling_action, NULL);
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

void _computer_manager_add(NVCOMPUTER *p)
{
    struct COMPUTER_MANAGER_T *cm = &computer_manager;
    cm->computer_list = g_list_append(cm->computer_list, p);
}

void nvcomputer_free(NVCOMPUTER *p)
{
    g_list_free_full(p->app_list, (GDestroyNotify)nvapp_free);
    free(p);
}

void nvapp_free(NVAPP *p)
{
    free(p);
}