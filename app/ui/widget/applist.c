#include "ui/launcher_window.h"
#include "backend/application_manager.h"
#include "backend/coverloader.h"
#include "stream/session.h"

#define LINKEDLIST_TYPE PAPP_LIST
#include "util/linked_list.h"

static bool _applist_item(struct nk_context *ctx, PSERVER_LIST node, PAPP_LIST cur, int cover_width, int cover_height, bool event_emitted);

bool launcher_applist(struct nk_context *ctx, PSERVER_LIST node, bool event_emitted)
{
    static struct nk_list_view list_view;
    struct nk_style_window winstyle = ctx->style.window;
    int app_len = linkedlist_len(node->apps);
    int colcount = 5, rowcount = app_len / colcount;
    if (app_len && app_len % colcount)
    {
        rowcount++;
    }

    // Row width of list item content
    int rowwidth = nk_widget_width(ctx) - winstyle.group_padding.x * 2 - winstyle.group_border * 2 - winstyle.scrollbar_size.x;
    int itemwidth = (rowwidth - winstyle.spacing.x * (colcount - 1)) / colcount;
    int coverwidth = itemwidth - winstyle.group_padding.x * 2 - winstyle.group_border * 2, coverheight = coverwidth / 3 * 4;
    int itemheight = coverheight + winstyle.group_padding.y * 2 + winstyle.group_border * 2;

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
                _applist_item(ctx, node, cur, coverwidth, coverheight, event_emitted);
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

bool _applist_item(struct nk_context *ctx, PSERVER_LIST node, PAPP_LIST cur,
                   int cover_width, int cover_height, bool event_emitted)
{
    nk_bool hovered = nk_widget_is_hovered(ctx);
    int item_height = nk_widget_height(ctx);
    if (nk_group_begin(ctx, cur->name, NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER))
    {
        struct nk_image *cover = coverloader_get(node->server, cur->id);
        bool running = node->server->currentGame == cur->id;
        nk_layout_space_begin(ctx, NK_STATIC, item_height, running ? 3 : 1);
        nk_layout_space_push(ctx, nk_rect(0, 0, cover_width, cover_height));
        if (cover)
        {
            nk_image(ctx, *cover);
        }
        else
        {
            nk_spacing(ctx, 1);
        }

        const int button_size = 24 * NK_UI_SCALE;
        int button_x = (cover_width - button_size) / 2;
        int button_spacing = 4 * NK_UI_SCALE;
        nk_layout_space_push(ctx, nk_rect(button_x, cover_height / 2 - button_size - button_spacing, button_size, button_size));
        nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_RIGHT);
        nk_layout_space_push(ctx, nk_rect(button_x, cover_height / 2 + button_spacing, button_size, button_size));
        nk_button_symbol(ctx, NK_SYMBOL_X);
        nk_layout_space_end(ctx);
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