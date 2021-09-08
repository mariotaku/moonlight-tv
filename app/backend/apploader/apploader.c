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

#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX

typedef struct {
    apploader_t *loader;
    apploader_cb cb;
    void *userdata;
    int code;
    APP_LIST *result;
    int result_count;
} apploader_task_t;

static apploader_task_t *apploader_task_create(apploader_t *loader, apploader_cb cb, void *userdata);

static int apploader_task_execute(apploader_task_t *task);

static void apploader_task_finish(apploader_task_t *task);

static int applist_name_comparator(PAPP_LIST p1, PAPP_LIST p2);

apploader_t *apploader_new(const SERVER_LIST *node) {
    apploader_t *loader = SDL_malloc(sizeof(apploader_t));
    SDL_memset(loader, 0, sizeof(apploader_t));
    loader->node = node;
    loader->apps = NULL;
    loader->apps_count = 0;
    return loader;
}

void apploader_load(apploader_t *loader, apploader_cb cb, void *userdata) {
    if (loader->status != APPLOADER_STATUS_IDLE) return;
    loader->status = APPLOADER_STATUS_LOADING;
    SDL_CreateThread((SDL_ThreadFunction) apploader_task_execute, "loadapps",
                     apploader_task_create(loader, cb, userdata));
}

void apploader_destroy(apploader_t *loader) {
    if (loader->apps) {
        for (int i = 0; i < loader->apps_count; i++) {
            SDL_free(loader->apps[i].name);
        }
        SDL_free(loader->apps);
    }
    SDL_free(loader);
}

static apploader_task_t *apploader_task_create(apploader_t *loader, apploader_cb cb, void *userdata) {
    apploader_task_t *task = SDL_malloc(sizeof(apploader_task_t));
    SDL_memset(task, 0, sizeof(apploader_task_t));
    task->loader = loader;
    task->cb = cb;
    task->userdata = userdata;
    return task;
}

static int apploader_task_execute(apploader_task_t *task) {
    PAPP_LIST ll = NULL;
    int ret;
    if ((ret = gs_applist(app_gs_client_obtain(), task->loader->node->server, &ll)) != GS_OK) {
        goto finish;
    }
    SDL_assert(ll);
    int result_count = applist_len(ll);
    APP_LIST *result = SDL_malloc(result_count * sizeof(APP_LIST));
    int index = 0;
    for (PAPP_LIST cur = ll; cur; cur = cur->next) {
        result[index] = *cur;
        result[index].next = (index + 1 < result_count) ? &result[index + 1] : NULL;
        index++;
    }
    applist_free(ll, (void (*)(APP_LIST *)) SDL_free);
    SDL_qsort(result, result_count, sizeof(APP_LIST), (int (*)(const void *, const void *)) applist_name_comparator);
    task->result = result;
    task->result_count = result_count;
    finish:
    task->code = ret;
    bus_pushaction((bus_actionfunc) apploader_task_finish, task);
    return ret;
}

static void apploader_task_finish(apploader_task_t *task) {
    apploader_t *loader = task->loader;
    if (task->code == GS_OK) {
        loader->apps = task->result;
        loader->apps_count = task->result_count;
    }
    loader->code = task->code;
    loader->status = APPLOADER_STATUS_IDLE;
    task->cb(loader, task->userdata);
    SDL_free(task);
}

static int applist_name_comparator(PAPP_LIST p1, PAPP_LIST p2) {
    return strcmp(p1->name, p2->name);
}