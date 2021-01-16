#include "ui/launcher_window.h"
#include "backend/application_manager.h"
#include "backend/coverloader.h"
#include "stream/session.h"

#define LINKEDLIST_TYPE APP_DLIST
#define LINKEDLIST_DOUBLE 1
#include "util/linked_list.h"

#include "res.h"
#include "main.h"

static bool _applist_item(struct nk_context *ctx, PSERVER_LIST node, PAPP_DLIST cur, int cover_width, int cover_height, bool event_emitted);
static void _applist_item_do_click(PSERVER_LIST node, PAPP_DLIST cur, int clicked);
static bool _applist_item_select(int offset);
static bool _cover_use_default(struct nk_image *img);

static PAPP_DLIST _hovered_app = NULL, _focused_app = NULL;
static PAPP_DLIST _applist_visible_start = NULL;
static int _applist_colcount = 5;

bool launcher_applist(struct nk_context *ctx, PSERVER_LIST node, bool event_emitted)
{
    static struct nk_list_view list_view;
    struct nk_style_window winstyle = ctx->style.window;
    int app_len = linkedlist_len(node->apps);
    int colcount = _applist_colcount, rowcount = app_len / colcount;
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
        PAPP_DLIST cur = linkedlist_nth(node->apps, startidx);
        _applist_visible_start = cur;
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

bool _applist_item(struct nk_context *ctx, PSERVER_LIST node, PAPP_DLIST cur,
                   int cover_width, int cover_height, bool event_emitted)
{
    nk_bool hovered = nk_widget_is_hovered(ctx), mouse_down = nk_widget_has_mouse_click_down(ctx, NK_BUTTON_LEFT, true);
    if (hovered)
    {
        _hovered_app = cur;
        // Clear key selection
        _focused_app = NULL;
    }
    static int click_down_id = -1, should_ignore_click = -1;
    if (mouse_down && click_down_id == -1)
    {
        click_down_id = cur->id;
        // If event_emitted is true, means there's combo opened. So next click event shoule be ignored
        if (event_emitted)
        {
            should_ignore_click = cur->id;
        }
    }
    bool running = node->server->currentGame == cur->id;
    // Don't react to grid click if there's action button
    int clicked = 0;
    int item_height = nk_widget_height(ctx);
    if (nk_group_begin(ctx, cur->name, NK_WINDOW_NO_SCROLLBAR))
    {
        struct nk_image *cover = coverloader_get(node, cur->id);
        nk_layout_space_begin(ctx, NK_STATIC, item_height, running ? 3 : 1);
        nk_layout_space_push(ctx, nk_rect(0, 0, cover_width, cover_height));
        if (_focused_app == cur)
        {
            nk_style_push_style_item(ctx, &ctx->style.button.normal, nk_style_item_color(nk_ext_colortable[NK_COLOR_BUTTON_HOVER]));
        }
        clicked = !running & nk_button_image(ctx, _cover_use_default(cover) ? launcher_default_cover : *cover);
        if (_focused_app == cur)
        {
            nk_style_pop_style_item(ctx);
        }

        const int button_size = 24 * NK_UI_SCALE;
        int button_x = (cover_width - button_size) / 2;
        int button_spacing = 4 * NK_UI_SCALE;
        nk_layout_space_push(ctx, nk_rect(button_x, cover_height / 2 - button_size - button_spacing, button_size, button_size));
        if (nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_RIGHT))
        {
            clicked = 1;
        }
        nk_layout_space_push(ctx, nk_rect(button_x, cover_height / 2 + button_spacing, button_size, button_size));
        if (nk_button_symbol(ctx, NK_SYMBOL_X))
        {
            clicked = -1;
        }
        nk_layout_space_end(ctx);
        // nk_label(ctx, cur->name, NK_TEXT_ALIGN_MIDDLE);
        nk_group_end(ctx);
    }
    // Captured a click event that should be ignored, reset state
    if (should_ignore_click == cur->id && !mouse_down)
    {
        clicked = 0;
        should_ignore_click = -1;
    }
    if (clicked)
    {
        if (should_ignore_click == -1 && click_down_id == cur->id)
        {
            event_emitted |= true;
            _applist_item_do_click(node, cur, clicked);
        }
        should_ignore_click = -1;
        click_down_id = -1;
    }
    return event_emitted;
}

void _applist_item_do_click(PSERVER_LIST node, PAPP_DLIST cur, int clicked)
{
    if (clicked == 1)
    {
        if (node->server->currentGame > 0 && node->server->currentGame != cur->id)
        {
            printf("Quit running game first\n");
        }
        else
        {
            streaming_begin(node, cur->id);
        }
    }
    else
    {
        computer_manager_quitapp(node);
    }
}

bool _applist_dispatch_navkey(struct nk_context *ctx, PSERVER_LIST node, NAVKEY navkey)
{
    switch (navkey)
    {
    case NAVKEY_LEFT:
        return _applist_item_select(-1);
    case NAVKEY_RIGHT:
        return _applist_item_select(1);
    case NAVKEY_UP:
        return _applist_item_select(-_applist_colcount);
    case NAVKEY_DOWN:
        return _applist_item_select(_applist_colcount);
    case NAVKEY_ENTER:
    {
        if (_focused_app)
        {
            _applist_item_do_click(node, _focused_app, 1);
        }
        return true;
    }
    case NAVKEY_BACK:
        request_exit();
    default:
        break;
    }
    return true;
}

bool _applist_item_select(int offset)
{
    if (_applist_visible_start == NULL)
    {
        return true;
    }
    if (_focused_app == NULL)
    {
        _focused_app = _applist_visible_start;
        _hovered_app = NULL;
        return true;
    }
    PAPP_DLIST item = linkedlist_nth(_focused_app, offset);
    if (item)
    {
        _focused_app = item;
        _hovered_app = NULL;
    }
    return true;
}

bool _cover_use_default(struct nk_image *img)
{
    return img == NULL || img->w == 0 || img->h == 0;
}