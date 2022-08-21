#include "priv.h"

#include <assert.h>

typedef struct pcmanager_listener_list {
    const pcmanager_listener_t *listener;

    void *userdata;

    struct pcmanager_listener_list *prev;
    struct pcmanager_listener_list *next;
} pcmanager_listener_list;

#define LINKEDLIST_IMPL
#define LINKEDLIST_MODIFIER static
#define LINKEDLIST_TYPE pcmanager_listener_list
#define LINKEDLIST_PREFIX listeners
#define LINKEDLIST_DOUBLE 1

#include "util/linked_list.h"

#undef LINKEDLIST_DOUBLE
#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX

static int pcmanager_callbacks_comparator(pcmanager_listener_list *p1, const void *p2);

void pcmanager_listeners_notify(pcmanager_t *manager, const uuidstr_t *uuid, pcmanager_notify_type_t type) {
    assert(SDL_ThreadID() == manager->thread_id);
    for (pcmanager_listener_list *cur = manager->listeners; cur != NULL;) {
        pcmanager_listener_list *next = cur->next;
        const pcmanager_listener_t *l = cur->listener;
        pcmanager_listener_fn fn = NULL;
        switch (type) {
            case PCMANAGER_NOTIFY_ADDED:
                fn = l->added;
                break;
            case PCMANAGER_NOTIFY_UPDATED:
                fn = l->updated;
                break;
            case PCMANAGER_NOTIFY_REMOVED:
                fn = l->removed;
                break;
            default:
                break;
        }
        if (fn) {
            fn(uuid, cur->userdata);
        }
        cur = next;
    }
}

void pcmanager_register_listener(pcmanager_t *manager, const pcmanager_listener_t *listener, void *userdata) {
    assert(manager);
    assert(listener);
    assert(SDL_ThreadID() == manager->thread_id);
    pcmanager_listener_list *node = listeners_new();
    node->listener = listener;
    node->userdata = userdata;
    manager->listeners = listeners_append(manager->listeners, node);
}

void pcmanager_unregister_listener(pcmanager_t *manager, const pcmanager_listener_t *listener) {
    assert(manager);
    assert(listener);
    assert(SDL_ThreadID() == manager->thread_id);
    pcmanager_listener_list *node = listeners_find_by(manager->listeners, listener, pcmanager_callbacks_comparator);
    if (!node)
        return;
    manager->listeners = listeners_remove(manager->listeners, node);
    free(node);
}

static int pcmanager_callbacks_comparator(pcmanager_listener_list *p1, const void *p2) {
    return p1->listener != p2;
}
