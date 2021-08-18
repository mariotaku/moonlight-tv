#include "manager.h"
#include "launcher/window.h"
#include "settings/window.h"

#include <memory.h>

typedef struct UIMANAGER_STACK
{
    UIMANAGER_WINDOW_CREATOR creator;
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

void uimanager_push(UIMANAGER_WINDOW_CREATOR creator)
{
    if (windows_stack_tail)
    {
        if (windows_stack_tail->_win)
        {
            lv_obj_del(windows_stack_tail->_win);
            windows_stack_tail->_win = NULL;
        }
    }
    PUIMANAGER_STACK item = uistack_new();
    item->creator = creator;
    item->_win = item->creator();
    windows_stack = uistack_append(windows_stack, item);
    windows_stack_tail = item;
}

void uimanager_pop()
{
    if (windows_stack_tail)
    {
        if (windows_stack_tail->_win)
        {
            lv_obj_del(windows_stack_tail->_win);
            windows_stack_tail->_win = NULL;
        }
        free(windows_stack_tail);
        PUIMANAGER_STACK item = windows_stack_tail->prev;
        if (item)
        {
            item->next = NULL;
            item->_win = item->creator();
        }
        windows_stack_tail = item;
    }
}

void ui_init()
{
    /*Create a window*/
    uimanager_push(launcher_win_create);
}

void ui_open_settings()
{
    uimanager_push(settings_win_create);
}