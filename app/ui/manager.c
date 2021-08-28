#include "manager.h"
#include "launcher/window.h"
#include "settings/window.h"

#include <memory.h>
#include <SDL.h>

typedef struct UIMANAGER_STACK {
    UIMANAGER_WINDOW_CREATOR creator;
    lv_obj_t *_parent;
    lv_obj_t *_win;
    struct UIMANAGER_STACK *prev;
    struct UIMANAGER_STACK *next;
} UIMANAGER_STACK, *PUIMANAGER_STACK;

#define LINKEDLIST_IMPL

#define LINKEDLIST_TYPE UIMANAGER_STACK
#define LINKEDLIST_PREFIX uistack
#define LINKEDLIST_DOUBLE 1

#include "util/linked_list.h"

#undef LINKEDLIST_DOUBLE
#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX

static PUIMANAGER_STACK windows_stack = NULL;
static PUIMANAGER_STACK windows_stack_tail = NULL;

static void uimanager_render_bg(lv_event_t *event);

void uimanager_push(lv_obj_t *parent, UIMANAGER_WINDOW_CREATOR creator, const void *args) {
    if (windows_stack_tail) {
        if (windows_stack_tail->_win) {
            lv_obj_del(windows_stack_tail->_win);
            windows_stack_tail->_win = NULL;
        }
    }
    PUIMANAGER_STACK item = uistack_new();
    item->creator = creator;
    item->_parent = parent;
    item->_win = item->creator(parent, args);
    windows_stack = uistack_append(windows_stack, item);
    windows_stack_tail = item;
}

void uimanager_pop() {
    if (windows_stack_tail) {
        if (windows_stack_tail->_win) {
            lv_obj_del(windows_stack_tail->_win);
            windows_stack_tail->_win = NULL;
        }
        free(windows_stack_tail);
        PUIMANAGER_STACK item = windows_stack_tail->prev;
        if (item) {
            item->next = NULL;
            item->_win = item->creator(item->_parent, NULL);
        }
        windows_stack_tail = item;
    }
}

void ui_init() {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_opa(scr, 0, 0);
    /*Create a window*/
    uimanager_push(scr, launcher_win_create, NULL);
}

void ui_open_settings() {
    uimanager_push(lv_scr_act(), settings_win_create, NULL);
}