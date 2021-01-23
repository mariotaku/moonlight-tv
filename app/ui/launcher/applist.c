#include "window.h"
#include "backend/application_manager.h"
#include "backend/coverloader.h"
#include "stream/session.h"

#include "res.h"
#include "util/bus.h"
#include "util/user_event.h"

static bool _applist_item(struct nk_context *ctx, PSERVER_LIST node, PAPP_DLIST cur, int cover_width, int cover_height, bool event_emitted);
static void _applist_item_do_click(PSERVER_LIST node, PAPP_DLIST cur, int clicked);
static bool _applist_item_select(PSERVER_LIST node, int offset);
static bool _cover_use_default(struct nk_image *img);

static PAPP_DLIST applist_hovered_item = NULL, applist_focused_request = NULL;
static PAPP_DLIST _applist_visible_start = NULL;
static struct nk_list_view list_view;
static int _applist_colcount = 5;
static int _applist_rowcount = 0;
static struct nk_vec2 applist_focused_item_center = {0, 0}, applist_focused_close_center = {0, 0};

bool launcher_applist(struct nk_context *ctx, PSERVER_LIST node, bool event_emitted)
{
    struct nk_style_window winstyle = ctx->style.window;
    int app_len = applist_len(node->apps);
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
    _applist_rowcount = rowcount;

    if (nk_list_view_begin(ctx, &list_view, "apps_list", 0, itemheight, rowcount))
    {
        nk_layout_row_dynamic(ctx, itemheight, colcount);
        int startidx = list_view.begin * colcount;
        PAPP_DLIST cur = applist_nth(node->apps, startidx);
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
        applist_hovered_item = cur;
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
    struct nk_rect item_bounds = nk_widget_bounds(ctx);
    int item_height = item_bounds.h;
    if (cur == applist_focused_request)
    {
        // Send mouse pointer to the item
        applist_focused_item_center.x = item_bounds.x + item_bounds.w / 2;
        applist_focused_item_center.y = item_bounds.y + item_bounds.h / 2;
        bus_pushevent(USER_FAKEINPUT_MOUSE_MOTION, &applist_focused_item_center, NULL);
        applist_focused_request = NULL;
    }
    if (nk_group_begin(ctx, cur->name, NK_WINDOW_NO_SCROLLBAR))
    {
        struct nk_image *cover = coverloader_get(node, cur->id, cover_width, cover_height);
        bool defcover = _cover_use_default(cover);
        nk_layout_space_begin(ctx, NK_STATIC, item_height, running ? 3 : (defcover ? 2 : 1));
        nk_layout_space_push(ctx, nk_rect(0, 0, cover_width, cover_height));
        clicked = !running & nk_button_image(ctx, defcover ? launcher_default_cover : *cover);
        if (defcover && !running)
        {
            int insetx = 8 * NK_UI_SCALE, insety = 6 * NK_UI_SCALE;
            nk_layout_space_push(ctx, nk_rect(insetx, insety, cover_width - insetx * 2, cover_height - insety * 2));
            nk_label(ctx, cur->name, NK_TEXT_CENTERED);
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
        return _applist_item_select(node, -1);
    case NAVKEY_RIGHT:
        return _applist_item_select(node, 1);
    case NAVKEY_UP:
        return _applist_item_select(node, -_applist_colcount);
    case NAVKEY_DOWN:
        return _applist_item_select(node, _applist_colcount);
    case NAVKEY_CLOSE:
        if (applist_focused_request)
        {
            _applist_item_do_click(node, applist_focused_request, -1);
        }
        return true;
    case NAVKEY_CONFIRM:
    case NAVKEY_ENTER:
        if (applist_focused_request)
        {
            _applist_item_do_click(node, applist_focused_request, 1);
        }
        return true;
    default:
        break;
    }
    return false;
}

bool _applist_item_select(PSERVER_LIST node, int offset)
{
    if (_applist_visible_start == NULL)
    {
        return true;
    }
    if (applist_focused_request == NULL)
    {
        if (applist_hovered_item == NULL)
        {
            // Select first visible item
            applist_focused_request = _applist_visible_start;
            return true;
        }
        else
        {
            applist_focused_request = applist_hovered_item;
        }
    }
    PAPP_DLIST item = applist_nth(applist_hovered_item, offset);
    if (item)
    {
        applist_focused_request = item;
        applist_hovered_item = NULL;
        if (_applist_rowcount)
        {
            int rowheight = list_view.total_height / _applist_rowcount;
            int index = applist_index(node->apps, item);
            if (index >= 0)
            {
                int row = index / _applist_colcount;
                *list_view.scroll_pointer = rowheight * row;
            }
        }
    }
    return true;
}

bool _cover_use_default(struct nk_image *img)
{
    return img == NULL || img->w == 0 || img->h == 0;
}