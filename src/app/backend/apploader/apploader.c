#include "apploader.h"

#include "app.h"
#include "errors.h"
#include "util/bus.h"
#include "lazy.h"
#include "refcounter.h"

#include <errno.h>

#define LINKEDLIST_IMPL

#define LINKEDLIST_MODIFIER static
#define LINKEDLIST_TYPE APP_LIST
#define LINKEDLIST_PREFIX applist

#include "linked_list.h"
#include "logging.h"

#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX

struct apploader_task_ctx_t {
    int code;
    const char *error;
    apploader_list_t *result;
    apploader_t *loader;
    const executor_task_t *task;
};

struct apploader_t {
    refcounter_t refcounter;
    app_t *app;
    uuidstr_t uuid;
    apploader_cb_t callback;
    lazy_t client;
    executor_t *executor;
    apploader_state_t state;
    const executor_task_t *task;
    void *userdata;
};

static void apploader_unref(apploader_t *loader);

static void apploader_free(apploader_t *loader);

static apploader_task_ctx_t *task_create(apploader_t *loader);

static int task_run(apploader_task_ctx_t *task);

static void task_finalize(apploader_task_ctx_t *task, int result);

static int applist_name_comparator(apploader_item_t *p1, apploader_item_t *p2);

static void task_callback(apploader_task_ctx_t *task);

static apploader_list_t *apps_create(const struct pclist_t *node, PAPP_LIST ll);

apploader_t *apploader_create(app_t *app, const uuidstr_t *uuid, const apploader_cb_t *cb, void *userdata) {
    apploader_t *loader = calloc(1, sizeof(apploader_t));
    refcounter_init(&loader->refcounter);
    lazy_init(&loader->client, (lazy_supplier) app_gs_client_new, app);
    loader->app = app;
    loader->executor = app->backend.executor;
    loader->callback = *cb;
    loader->userdata = userdata;
    loader->uuid = *uuid;
    return loader;
}

void apploader_load(apploader_t *loader) {
    executor_task_state_t state = executor_task_state(loader->executor, loader->task);
    if (state == EXECUTOR_TASK_STATE_PENDING || state == EXECUTOR_TASK_STATE_ACTIVE) {
        return;
    }
    loader->state = APPLOADER_STATE_LOADING;
    if (loader->callback.start != NULL) {
        loader->callback.start(loader->userdata);
    }
    apploader_task_ctx_t *ctx = task_create(loader);
    const executor_task_t *task = executor_submit(loader->executor, (executor_action_cb) task_run,
                                                  (executor_cleanup_cb) task_finalize, ctx);
    commons_log_debug("AppLoader", "[loader %p] task start, task=%p", loader, task);
    ctx->task = task;
    loader->task = task;
}

void apploader_cancel(apploader_t *loader) {
    commons_log_debug("AppLoader", "[loader %p] task cancel, task=%p", loader, loader->task);
    executor_cancel(loader->executor, loader->task);
    loader->task = NULL;
}

void apploader_destroy(apploader_t *loader) {
    memset(&loader->callback, 0, sizeof(apploader_cb_t));
    apploader_unref(loader);
}

apploader_state_t apploader_state(apploader_t *loader) {
    return loader->state;
}

static void apploader_unref(apploader_t *loader) {

    if (!refcounter_unref(&loader->refcounter)) {
        return;
    }
    apploader_free(loader);
}

static void apploader_free(apploader_t *loader) {
    commons_log_debug("AppLoader", "[loader %p] free()", loader);
    GS_CLIENT client = lazy_deinit(&loader->client);
    if (client != NULL) {
        gs_destroy(client);
    }
    refcounter_destroy(&loader->refcounter);
    free(loader);
}

static apploader_task_ctx_t *task_create(apploader_t *loader) {
    apploader_task_ctx_t *task = calloc(1, sizeof(apploader_task_ctx_t));
    refcounter_ref(&loader->refcounter);
    task->loader = loader;
    return task;
}

static int task_run(apploader_task_ctx_t *task) {
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
    applist_free(ll, (applist_nodefree_fn) free);
    task->result = result;
    finish:
    task->code = ret;
    task->error = error;
    return ret;
}

static void task_finalize(apploader_task_ctx_t *task, int result) {
    commons_log_debug("AppLoader", "[loader %p] task finalize. result=%d", task->loader, result);
    if (result == ECANCELED) {
        if (task->result != NULL) {
            apploader_list_free(task->result);
        }
    } else if (app_bus_post(task->loader->app, (bus_actionfunc) task_callback, task)) {
        return;
    }
    apploader_unref(task->loader);
    free(task);
}

static void task_callback(apploader_task_ctx_t *task) {
    apploader_t *loader = task->loader;
    commons_log_debug("AppLoader", "[loader %p] task callback", loader);
    if (task->code == GS_OK) {
        loader->state = APPLOADER_STATE_IDLE;
        if (loader->callback.data != NULL) {
            loader->callback.data(task->result, loader->userdata);
        }
    } else {
        loader->state = APPLOADER_STATE_ERROR;
        if (loader->callback.error != NULL) {
            loader->callback.error(task->code, task->error, loader->userdata);
        }
    }
    apploader_unref(loader);
    free(task);
}

static apploader_list_t *apps_create(const struct pclist_t *node, PAPP_LIST ll) {
    int count = applist_len(ll);
    apploader_list_t *result = malloc(sizeof(apploader_list_t) + count * sizeof(apploader_item_t));
    result->count = count;
    result->items = (apploader_item_t *) ((void *) result + sizeof(apploader_list_t));
    int index = 0;
    for (PAPP_LIST cur = ll; cur; cur = cur->next) {
        apploader_item_t *item = &result->items[index];
        memcpy(&item->base, cur, sizeof(APP_LIST));
        item->base.next = NULL;
        item->fav = pcmanager_node_is_app_favorite(node, cur->id);
        item->hidden = pcmanager_node_is_app_hidden(node, cur->id);
        index++;
    }
    qsort(result->items, result->count, sizeof(apploader_item_t),
              (int (*)(const void *, const void *)) applist_name_comparator);
    return result;
}

void apploader_list_free(apploader_list_t *list) {
    if (!list) { return; }
    for (int i = 0; i < list->count; i++) {
        free(list->items[i].base.name);
    }
    free(list);
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
    if (p1->hidden != p2->hidden) {
        return p1->hidden ? 1 : -1;
    }
    int extra = p2->fav * 1000 - p1->fav * 1000;
    int namecmp = strcoll(p1->base.name, p2->base.name);
    if (namecmp > 0) {
        return 1 + extra;
    } else if (namecmp < 0) {
        return -1 + extra;
    }
    return extra;
}

