#include "window.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "ui/root.h"

#include "util/bus.h"
#include "util/user_event.h"

#define WINDOW_TITLE "Settings"

enum settings_entries
{
    ENTRY_NONE = -1,
    ENTRY_RES_FPS = 0,
    ENTRY_BITRATE,
    ENTRY_SOPS,
    ENTRY_LOCALAUDIO,
    ENTRY_VIEWONLY,
#if HAS_WEBOS_SETTINGS
    ENTRY_WEBOS_VDEC,
    ENTRY_WEBOS_SDLAUD,
#endif
    ENTRY_COUNT
};

typedef void (*settings_panel_render)(struct nk_context *);
typedef bool (*settings_panel_navkey)(struct nk_context *, NAVKEY navkey, NAVKEY_STATE state, uint32_t timestamp);
typedef int (*settings_panel_itemcount)();

struct settings_pane
{
    const char *title;
    settings_panel_render render;
    settings_panel_navkey navkey;
    settings_panel_itemcount itemcount;
};

static enum settings_entries _selected_entry = ENTRY_NONE;
static void _pane_select_offset(int offset);
static bool _pane_dispatch_navkey(struct nk_context *ctx, int pane_index, NAVKEY navkey, NAVKEY_STATE state, uint32_t timestamp);
static int _pane_itemcount(int index);

void _settings_nav(struct nk_context *ctx);
void _settings_pane_basic(struct nk_context *ctx);
bool _settings_pane_basic_navkey(struct nk_context *ctx, NAVKEY navkey, NAVKEY_STATE state, uint32_t timestamp);
int _settings_pane_basic_itemcount();

void _settings_pane_host(struct nk_context *ctx);
int _settings_pane_host_itemcount();

void _settings_pane_mouse(struct nk_context *ctx);
void _settings_pane_advanced(struct nk_context *ctx);
void settings_statbar(struct nk_context *ctx);
void settings_set_pane_focused(bool focused);

void _pane_basic_open();
void _pane_host_open();

const struct settings_pane settings_panes[] = {
    {"Basic Settings", _settings_pane_basic, _settings_pane_basic_navkey, _settings_pane_basic_itemcount},
    {"Host Settings", _settings_pane_host, NULL, _settings_pane_host_itemcount},
    {"Mouse Settings", _settings_pane_mouse, NULL},
    {"Advanced Settings", _settings_pane_advanced, NULL},
};
#define settings_panes_size ((int)(sizeof(settings_panes) / sizeof(struct settings_pane)))

static int selected_pane_index = 0;
bool settings_pane_focused = false;
int settings_hovered_item = -1;
int settings_item_hover_request = -1;
struct nk_vec2 settings_focused_item_center = {0, 0};

void settings_window_init(struct nk_context *ctx)
{
}

bool settings_window_open()
{
    if (ui_settings_showing)
    {
        return false;
    }
    ui_settings_showing = true;
    _pane_basic_open();
    _pane_host_open();
    return true;
}

bool settings_window_close()
{
    if (!ui_settings_showing)
    {
        return false;
    }
    ui_settings_showing = false;
    settings_save(app_configuration);
    return true;
}

bool settings_window(struct nk_context *ctx)
{
    struct nk_rect s = nk_rect(0, 0, ui_display_width, ui_display_height);
    nk_style_push_vec2(ctx, &ctx->style.window.padding, nk_vec2_s(20, 15));
    nk_style_push_vec2(ctx, &ctx->style.window.scrollbar_size, nk_vec2_s(2, 0));
    if (nk_begin(ctx, WINDOW_TITLE, s, NK_WINDOW_NO_SCROLLBAR))
    {
        struct nk_vec2 content_size = nk_window_get_content_inner_size(ctx);
        float content_height = content_size.y;

        nk_layout_row_dynamic_s(ctx, UI_TITLE_BAR_HEIGHT_DP, 1);
        content_height -= nk_widget_height(ctx);
        content_height -= ctx->style.window.spacing.y;
        nk_label(ctx, "Settings", NK_TEXT_LEFT);

        content_height -= UI_BOTTOM_BAR_HEIGHT_DP * NK_UI_SCALE;
        content_height -= ctx->style.window.spacing.y;
        struct nk_rect content_bounds = nk_widget_bounds(ctx);

        static const float pane_ratio[] = {0.33, 0.67};
        nk_layout_row(ctx, NK_DYNAMIC, content_height, 2, pane_ratio);
        if (nk_group_begin(ctx, "settings_nav", 0))
        {
            nk_layout_row_dynamic_s(ctx, 30, 1);
            for (int i = 0; i < settings_panes_size; i++)
            {
                nk_bool selected = i == selected_pane_index;
                if (nk_selectable_label(ctx, settings_panes[i].title, NK_TEXT_LEFT, &selected))
                {
                    selected_pane_index = i;
                }
            }
            nk_group_end(ctx);
        }
        nk_style_push_style_item(ctx, &ctx->style.window.fixed_background, nk_style_item_color(nk_rgb(40, 40, 40)));
        nk_style_push_vec2(ctx, &ctx->style.window.group_padding, nk_vec2_s(10, 10));
        if (nk_group_begin_titled(ctx, "settings_pane", "Settings", 0))
        {
            if (settings_panes[selected_pane_index].render)
            {
                settings_panes[selected_pane_index].render(ctx);
            }
            if (settings_item_hover_request >= 0)
            {
                bus_pushevent(USER_FAKEINPUT_MOUSE_MOTION, &settings_focused_item_center, NULL);
                settings_item_hover_request = -1;
            }
            nk_group_end(ctx);
        }
        nk_style_pop_vec2(ctx);
        nk_style_pop_style_item(ctx);

        nk_stroke_line(&ctx->current->buffer, content_bounds.x, content_bounds.y, content_bounds.x + content_bounds.w,
                       content_bounds.y, 1 * NK_UI_SCALE, ctx->style.text.color);

        settings_statbar(ctx);
    }
    nk_end(ctx);
    nk_style_pop_vec2(ctx);
    nk_style_pop_vec2(ctx);
    // Why Nuklear why, the button looks like "close" but it actually "hide"
    if (nk_window_is_hidden(ctx, WINDOW_TITLE))
    {
        nk_window_close(ctx, WINDOW_TITLE);
        return false;
    }
    return true;
}

bool settings_window_dispatch_navkey(struct nk_context *ctx, NAVKEY navkey, NAVKEY_STATE state, uint32_t timestamp)
{
    switch (navkey)
    {
    case NAVKEY_CANCEL:
        if (state)
        {
            return true;
        }
        if (settings_pane_focused)
        {
            settings_set_pane_focused(false);
        }
        else
        {
            nk_window_show(ctx, WINDOW_TITLE, false);
        }
        break;
    case NAVKEY_UP:
        if (settings_pane_focused)
        {
            if (_pane_dispatch_navkey(ctx, selected_pane_index, navkey, state, timestamp))
            {
                break;
            }
            else if (!navkey_intercept_repeat(state, timestamp))
            {
                settings_pane_item_offset(-1);
            }
        }
        else if (!navkey_intercept_repeat(state, timestamp))
        {
            _pane_select_offset(-1);
        }
        break;
    case NAVKEY_DOWN:
        if (settings_pane_focused)
        {
            if (_pane_dispatch_navkey(ctx, selected_pane_index, navkey, state, timestamp))
            {
                break;
            }
            else if (!navkey_intercept_repeat(state, timestamp))
            {
                settings_pane_item_offset(1);
            }
        }
        else if (!navkey_intercept_repeat(state, timestamp))
        {
            _pane_select_offset(1);
        }
        break;
    case NAVKEY_LEFT:
        if (settings_pane_focused)
        {
            if (_pane_dispatch_navkey(ctx, selected_pane_index, navkey, state, timestamp))
            {
                break;
            }
            else if (state == NAVKEY_STATE_UP)
            {
                settings_set_pane_focused(false);
            }
        }
        break;
    case NAVKEY_RIGHT:
    case NAVKEY_CONFIRM:
        if (settings_pane_focused)
        {
            _pane_dispatch_navkey(ctx, selected_pane_index, navkey, state, timestamp);
        }
        else if (state == NAVKEY_STATE_UP)
        {

            settings_set_pane_focused(true);
        }
        break;
    default:
        break;
    }
    return true;
}

bool _pane_dispatch_navkey(struct nk_context *ctx, int pane_index, NAVKEY navkey, NAVKEY_STATE state, uint32_t timestamp)
{
    return settings_panes[pane_index].navkey && settings_panes[pane_index].navkey(ctx, navkey, state, timestamp);
}

void _pane_select_offset(int offset)
{
    int new_index = selected_pane_index + offset;
    if (new_index < 0)
    {
        selected_pane_index = 0;
    }
    else if (new_index >= settings_panes_size)
    {
        selected_pane_index = settings_panes_size - 1;
    }
    else
    {
        selected_pane_index = new_index;
    }
}

void settings_pane_item_offset(int offset)
{
    if (!settings_panes[selected_pane_index].itemcount)
    {
        return;
    }
    int itemcount = settings_panes[selected_pane_index].itemcount();
    int new_index = settings_hovered_item + offset;
    if (new_index < 0)
    {
        settings_item_hover_request = 0;
    }
    else if (new_index >= itemcount)
    {
        settings_item_hover_request = itemcount - 1;
    }
    else
    {
        settings_item_hover_request = new_index;
    }
}

int _pane_itemcount(int index)
{
    if (!settings_panes[index].itemcount)
    {
        return 0;
    }
    return settings_panes[index].itemcount();
}

void settings_item_update_selected_bounds(struct nk_context *ctx, int index, struct nk_rect *bounds)
{
    if (nk_widget_is_hovered(ctx))
    {
        settings_hovered_item = index;
    }
    if (settings_item_hover_request == index)
    {
        *bounds = nk_widget_bounds(ctx);

        settings_focused_item_center.x = nk_rect_center_x(*bounds);
        settings_focused_item_center.y = nk_rect_center_y(*bounds);
    }
}

void settings_set_pane_focused(bool focused)
{
    settings_pane_focused = focused;
    if (!focused)
    {
        settings_item_hover_request = -1;
        settings_focused_item_center.x = 0;
        settings_focused_item_center.y = 0;
        bus_pushevent(USER_FAKEINPUT_MOUSE_MOTION, &settings_focused_item_center, NULL);
    }
    else
    {
        settings_item_hover_request = 0;
    }
}