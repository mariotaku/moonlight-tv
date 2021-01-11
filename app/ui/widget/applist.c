#include "ui/launcher_window.h"
#include "backend/application_manager.h"
#include "backend/coverloader.h"
#include "stream/session.h"

#define LINKEDLIST_TYPE PAPP_LIST
#include "util/linked_list.h"

bool cw_application_list(struct nk_context *ctx, PSERVER_LIST node, bool event_emitted)
{
    static struct nk_list_view list_view;
    int app_len = linkedlist_len(node->apps);
    int listwidth = nk_widget_width(ctx);
    int itemwidth = 60 * NK_UI_SCALE, itemheight = 150 * NK_UI_SCALE;
    int colcount = 5, rowcount = app_len / colcount;
    if (app_len && app_len % colcount)
    {
        rowcount++;
    }
    if (nk_list_view_begin(ctx, &list_view, "apps_list", NK_WINDOW_BORDER, itemheight, rowcount))
    {
        nk_layout_row_dynamic(ctx, itemheight, colcount);
        int startidx = list_view.begin * colcount;
        PAPP_LIST cur = linkedlist_nth(node->apps, startidx);
        for (int row = 0; row < list_view.count; row++)
        {
            int col;
            for (col = 0; col < colcount && cur != NULL; col++, cur = cur->next)
            {
                struct nk_vec2 grid_size = nk_widget_size(ctx);
                if (nk_group_begin(ctx, cur->name, NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER))
                {
                    nk_layout_row_dynamic(ctx, grid_size.y, 1);
                    struct nk_image *cover = coverloader_get(node->server, cur->id);
                    if (cover)
                    {
                        nk_image(ctx, *cover);
                    }
                    else
                    {
                        nk_spacing(ctx, 1);
                    }
                    
                    // nk_label(ctx, cur->name, NK_TEXT_ALIGN_MIDDLE);
                    if (false)
                    {
                        if (!event_emitted)
                        {
                            event_emitted |= true;
                            streaming_begin(node->server, cur->id);
                        }
                    }
                    nk_group_end(ctx);
                }
            }
            if (col < colcount)
            {
                nk_spacing(ctx, colcount - col);
            }
        }

        nk_list_view_end(&list_view);
    }
    return event_emitted;
}