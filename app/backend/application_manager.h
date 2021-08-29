#pragma once

#include "libgamestream/client.h"

#include "backend/types.h"

typedef struct APPMANAGER_CALLBACKS {
    void (*updated)(void *, PSERVER_LIST);

    void *userdata;
    struct APPMANAGER_CALLBACKS *prev;
    struct APPMANAGER_CALLBACKS *next;
} APPMANAGER_CALLBACKS, *PAPPMANAGER_CALLBACKS;

#ifdef APPMANAGER_IMPL
#define LINKEDLIST_IMPL
#endif

#define LINKEDLIST_TYPE APPMANAGER_CALLBACKS
#define LINKEDLIST_PREFIX appmanager_callbacks
#define LINKEDLIST_DOUBLE 1

#include "util/linked_list.h"

#undef LINKEDLIST_DOUBLE
#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX

/**
 * @brief Initialize application manager context
 * 
 */
void application_manager_init();

/**
 * @brief Free all allocated memories
 * 
 */
void application_manager_destroy();

/**
 * @brief Load applications list for server in background
 * 
 * @param node 
 */
void application_manager_load(PSERVER_LIST node);

bool application_manager_dispatch_userevent(int which, void *data1, void *data2);

void appmanager_register_callbacks(PAPPMANAGER_CALLBACKS callbacks);

void appmanager_unregister_callbacks(PAPPMANAGER_CALLBACKS callbacks);