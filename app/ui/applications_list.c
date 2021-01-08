#include "launcher_window.h"
#include "backend/application_manager.h"
#include "stream/session.h"

#define LINKEDLIST_TYPE PAPP_LIST
#include "util/linked_list.h"

bool cw_application_list(struct nk_context *ctx, PSERVER_LIST node, bool event_emitted)
{
    struct nk_list_view list_view;
    int app_len = linkedlist_len(node->apps);
    if (nk_list_view_begin(ctx, &list_view, "apps_list", NK_WINDOW_BORDER, 25, app_len))
    {
        nk_layout_row_dynamic(ctx, 25, 1);
        PAPP_LIST cur = linkedlist_nth(node->apps, list_view.begin);

        for (int i = 0; i < list_view.count; ++i, cur = cur->next)
        {
            if (nk_list_item_label(ctx, cur->name, NK_TEXT_ALIGN_LEFT))
            {
                if (!event_emitted)
                {
                    event_emitted |= true;
                    streaming_begin(node->server, cur->id);
                }
            }
        }
        nk_list_view_end(&list_view);
    }
    return event_emitted;
}