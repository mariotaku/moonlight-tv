//
// Created by Mariotaku on 2021/09/07.
//

#include "apploader.h"

#include "app.h"
#include "errors.h"
#include "util/bus.h"
#include "lazy.h"
#include "refcounter.h"

#include <errno.h>
#include <SDL.h>

#define LINKEDLIST_IMPL

#define LINKEDLIST_MODIFIER static
#define LINKEDLIST_TYPE APP_LIST
#define LINKEDLIST_PREFIX applist

#include "linked_list.h"

#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX

struct apploader_task_t {
    int code;
    const char *error;
    apploader_list_t *result;
    apploader_t *loader;
    const executor_task_t *task;
};

struct apploader_t {
    uuidstr_t uuid;
    apploader_cb_t callback;
    lazy_t client;
    executor_t *executor;
    apploader_state_t state;
    void *userdata;
};

static void apploader_free(executor_t *executor, int wait);

static apploader_task_t *task_create(apploader_t *loader);

static int task_run(apploader_task_t *task);

static void task_finalize(apploader_task_t *task, int result);

static int applist_name_comparator(apploader_item_t *p1, apploader_item_t *p2);

static void task_callback(apploader_task_t *task);

static apploader_list_t *apps_create(const struct pclist_t *node, PAPP_LIST ll);

static void loader_thread_wait(SDL_Thread *thread);

apploader_t *apploader_create(const uuidstr_t *uuid, const apploader_cb_t *cb, void *userdata) {
    apploader_t *loader = SDL_malloc(sizeof(apploader_t));
    SDL_memset(loader, 0, sizeof(apploader_t));
    lazy_init(&loader->client, (lazy_supplier) app_gs_client_new, NULL);
    loader->executor = executor_create("apploader", apploader_free);
    loader->callback = *cb;
    loader->userdata = userdata;
    loader->uuid = *uuid;
    executor_set_userdata(loader->executor, loader);
    return loader;
}

void apploader_load(apploader_t *loader) {
    if (executor_is_active(loader->executor)) {
        return;
    }
    loader->state = APPLOADER_STATE_LOADING;
    loader->callback.start(loader->userdata);
    apploader_task_t *task = task_create(loader);
    task->task = executor_execute(loader->executor, (executor_action_cb) task_run,
                                  (executor_cleanup_cb) task_finalize, task);
}

void apploader_cancel(apploader_t *loader) {
    executor_cancel(loader->executor, NULL);
}

void apploader_destroy(apploader_t *loader) {
    executor_destroy(loader->executor, 0);
}

apploader_state_t apploader_state(apploader_t *loader) {
    return loader->state;
}

static void apploader_free(executor_t *executor, int wait) {
    SDL_assert(!wait);
    apploader_t *loader = executor_get_userdata(executor);
    void *thread = executor_get_thread_handle(executor);
    GS_CLIENT client = lazy_deinit(&loader->client);
    if (client != NULL) {
        gs_destroy(client);
    }
    SDL_free(loader);
    bus_pushaction((bus_actionfunc) loader_thread_wait, thread);
}

static apploader_task_t *task_create(apploader_t *loader) {
    apploader_task_t *task = SDL_calloc(1, sizeof(apploader_task_t));
    task->loader = loader;
    return task;
}

static int task_run(apploader_task_t *task) {
    int ret = GS_OK;
    const char *error = NULL;
    const pclist_t *node = pcmanager_node(pcmanager, &task->loader->uuid);
    if (node == NULL) {
        ret = GS_ERROR;
        goto finish;
    }
    PAPP_LIST ll = NULL;
    GS_CLIENT client = lazy_obtain(&task->loader->client);
    if ((ret = gs_applist(client, node->server, &ll)) != GS_OK) {
        gs_get_error(&error);
        goto finish;
    }
    if (ll == NULL) {
        ret = GS_ERROR;
        goto finish;
    }
    apploader_list_t *result = apps_create(node, ll);
    applist_free(ll, (applist_nodefree_fn) SDL_free);
    task->result = result;
    finish:
    task->code = ret;
    task->error = error;
    return ret;
}

static void task_finalize(apploader_task_t *task, int result) {
    if (result == ECANCELED) {
        if (task->result != NULL) {
            apploader_list_free(task->result);
        }
    } else {
        bus_pushaction_sync((bus_actionfunc) task_callback, task);
    }
    SDL_free(task);
}

static void task_callback(apploader_task_t *task) {
    apploader_t *loader = task->loader;
    if (executor_is_destroyed(loader->executor)) {
        if (task->result != NULL) {
            apploader_list_free(task->result);
        }
        return;
    }
    if (task->code == GS_OK) {
        loader->state = APPLOADER_STATE_IDLE;
        loader->callback.data(task->result, loader->userdata);
    } else {
        loader->state = APPLOADER_STATE_ERROR;
        loader->callback.error(task->code, task->error, loader->userdata);
    }
}

static apploader_list_t *apps_create(const struct pclist_t *node, PAPP_LIST ll) {
    int count = applist_len(ll);
    apploader_list_t *result = SDL_malloc(sizeof(apploader_list_t) + count * sizeof(apploader_item_t));
    result->count = count;
    result->items = (apploader_item_t *) ((void *) result + sizeof(apploader_list_t));
    int index = 0;
    for (PAPP_LIST cur = ll; cur; cur = cur->next) {
        apploader_item_t *item = &result->items[index];
        SDL_memcpy(&item->base, cur, sizeof(APP_LIST));
        item->base.next = NULL;
        item->fav = pcmanager_node_is_app_favorite(node, cur->id);
        index++;
    }
    SDL_qsort(result->items, result->count, sizeof(apploader_item_t),
              (int (*)(const void *, const void *)) applist_name_comparator);
    return result;
}

void apploader_list_free(apploader_list_t *list) {
    if (!list) { return; }
    for (int i = 0; i < list->count; i++) {
        SDL_free(list->items[i].base.name);
    }
    SDL_free(list);
}

const apploader_item_t *apploader_list_item_by_id(const apploader_list_t *list, int id) {
    for (int i = 0; i < list->count; ++i) {
        if (list->items[i].base.id == id) {
            return &list->items[i];
        }
    }
    return NULL;
}

static int applist_name_comparator(apploader_item_t *p1, apploader_item_t *p2) {
    int extra = p2->fav * 1000 - p1->fav * 1000;
    int namecmp = strcoll(p1->base.name, p2->base.name);
    if (namecmp > 0) {
        return 1 + extra;
    } else if (namecmp < 0) {
        return -1 + extra;
    }
    return extra;
}

static void loader_thread_wait(SDL_Thread *thread) {
    SDL_WaitThread(thread, NULL);
}