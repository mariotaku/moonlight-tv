#pragma once

#include "../pcmanager.h"
#include "executor.h"
#include "uuidstr.h"
#include <SDL.h>

typedef struct app_t app_t;
typedef struct pcmanager_listener_list pcmanager_listener_list;

typedef enum pcmanager_notify_type_t {
    PCMANAGER_NOTIFY_ADDED,
    PCMANAGER_NOTIFY_UPDATED,
    PCMANAGER_NOTIFY_REMOVED,
} pcmanager_notify_type_t;

typedef struct {
    pcmanager_t *manager;
    SERVER_STATE state;
    SERVER_DATA *server;
    uuidstr_t uuid;
} pclist_update_context_t;

struct pcmanager_t {
    app_t *app;
    SDL_threadID thread_id;
    executor_t *executor;
    pclist_t *servers;
    SDL_mutex *servers_lock;
    pcmanager_listener_list *listeners;
};

void serverdata_free(PSERVER_DATA data);

PSERVER_DATA serverdata_new();

PSERVER_DATA serverdata_clone(const SERVER_DATA *src);

void pcmanager_load_known_hosts(pcmanager_t *manager);

void pcmanager_save_known_hosts(pcmanager_t *manager);
