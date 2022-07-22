//
// Created by Mariotaku on 2021/09/07.
//

#include "apploader.h"

#include "app.h"
#include "errors.h"
#include "util/bus.h"

#include <SDL.h>

#define LINKEDLIST_IMPL

#define LINKEDLIST_MODIFIER static
#define LINKEDLIST_TYPE APP_LIST
#define LINKEDLIST_PREFIX applist

#include "util/linked_list.h"
#include "backend/pcmanager/priv.h"

#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX

struct apploader_task_t {
    apploader_t *loader;
    apploader_cb cb;
    void *userdata;
    int code;
    const char *error;
    apploader_list_t *result;
    SDL_Thread *thread;
    bool cancelled;
};

static apploader_task_t *apploader_task_create(apploader_t *loader, apploader_cb cb, void *userdata);

static int apploader_task_execute(apploader_task_t *task);

static void apploader_task_finish(apploader_task_t *task);

static void apploader_list_free(apploader_list_t *list);

static int applist_name_comparator(apploader_item_t *p1, apploader_item_t *p2);

apploader_t *apploader_new(const SERVER_LIST *node) {
    apploader_t *loader = SDL_malloc(sizeof(apploader_t));
    SDL_memset(loader, 0, sizeof(apploader_t));
    refcounter_init(&loader->refcounter);
    loader->uuid = strdup(node->server->uuid);
    loader->apps = NULL;
    return loader;
}

void apploader_load(apploader_t *loader, apploader_cb cb, void *userdata) {
    if (loader->state != APPLOADER_STATE_IDLE) return;
    loader->state = APPLOADER_STATE_LOADING;
    apploader_task_t *task = apploader_task_create(loader, cb, userdata);
    refcounter_ref(&loader->refcounter);
    task->thread = SDL_CreateThread((SDL_ThreadFunction) apploader_task_execute, "loadapps", task);
    loader->task = task;
}

void apploader_unref(apploader_t *loader) {
    if (loader->task) {
        SDL_assert(!loader->task->cancelled);
        loader->task->cancelled = true;
    }
    if (!refcounter_unref(&loader->refcounter)) {
        return;
    }
    apploader_list_free(loader->apps);
    free(loader->uuid);
    refcounter_destroy(&loader->refcounter);
    SDL_free(loader);
}


static apploader_task_t *apploader_task_create(apploader_t *loader, apploader_cb cb, void *userdata) {
    apploader_task_t *task = SDL_malloc(sizeof(apploader_task_t));
    SDL_memset(task, 0, sizeof(apploader_task_t));
    task->loader = loader;
    task->cb = cb;
    task->userdata = userdata;
    task->cancelled = false;
    return task;
}

static int apploader_task_execute(apploader_task_t *task) {
    int ret = GS_OK;
    const char *error = NULL;
    GS_CLIENT client = NULL;
    if (task->cancelled) {
        goto finish;
    }
    PSERVER_LIST node = pcmanager_find_by_uuid(pcmanager, task->loader->uuid);
    if (node == NULL) {
        ret = GS_ERROR;
        goto finish;
    }
    PAPP_LIST ll = NULL;
    client = app_gs_client_new();
    if ((ret = gs_applist(client, node->server, &ll)) != GS_OK) {
        error = gs_error;
        goto finish;
    }
    if (task->cancelled) {
        goto finish;
    }
    // The app list is empty
    if (ll == NULL) {
        task->result = NULL;
        goto finish;
    }
    int count = applist_len(ll);
    apploader_list_t *result = SDL_malloc(sizeof(apploader_list_t) + (count - 1) * sizeof(apploader_item_t));
    result->count = count;
    apploader_item_t *items = &result->items;
    int index = 0;
    for (PAPP_LIST cur = ll; cur; cur = cur->next) {
        apploader_item_t *item = &items[index];
        SDL_memcpy(item, cur, sizeof(APP_LIST));
        item->base.next = NULL;
        item->fav = pcmanager_is_favorite(node, cur->id);
        index++;
    }
    applist_free(ll, (void (*)(APP_LIST *)) SDL_free);
    SDL_qsort(items, result->count, sizeof(apploader_item_t),
              (int (*)(const void *, const void *)) applist_name_comparator);
    task->result = result;
    finish:
    task->code = ret;
    task->error = error;
    gs_destroy(client);
    bus_pushaction((bus_actionfunc) apploader_task_finish, task);
    return ret;
}

static void apploader_task_finish(apploader_task_t *task) {
    apploader_t *loader = task->loader;
    if (task->cancelled) {
        loader->task = NULL;
        apploader_unref(loader);
        return;
    }
    apploader_list_t *old_apps = loader->apps;
    if (task->code == GS_OK) {
        loader->apps = task->result;
    } else {
        loader->apps = NULL;
    }
    loader->code = task->code;
    loader->error = task->error;
    loader->state = APPLOADER_STATE_IDLE;
    loader->task = NULL;
    task->cb(loader, task->userdata);
    apploader_list_free(old_apps);
    SDL_free(task);
    apploader_unref(loader);
}

static void apploader_list_free(apploader_list_t *list) {
    if (!list) return;
    apploader_item_t *items = &list->items;
    for (int i = 0; i < list->count; i++) {
        SDL_free(items[i].base.name);
    }
    SDL_free(list);
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