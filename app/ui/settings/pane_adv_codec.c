#include "window.h"
#include "priv.h"
#include "ui/root.h"

#include <stddef.h>
#include <stdio.h>

#include "util/bus.h"
#include "util/user_event.h"

#include "stream/platform.h"

struct _ch_option
{
    int configuration;
    char name[16];
};

static const struct _ch_option _supported_ch[] = {
    {AUDIO_CONFIGURATION_STEREO, "Stereo"},
    {AUDIO_CONFIGURATION_51_SURROUND, "5.1 Surround"},
};
#define _supported_ch_len sizeof(_supported_ch) / sizeof(struct _ch_option)

static struct
{
    int combo;
    int item, count;
    int request;
} combo_hovered_item;

static struct nk_vec2 combo_focused_center;

static bool combo_item_select(int offset);

static bool _vdec_combo(struct nk_context *ctx, DECODER selplat);

static bool _render(struct nk_context *ctx, bool *showing_combo)
{
    static struct nk_rect item_bounds = {0, 0, 0, 0};
    int item_index = 0;

    nk_layout_row_dynamic_s(ctx, 25, 1);
    nk_label(ctx, "Video decoder", NK_TEXT_LEFT);
    settings_item_update_selected_bounds(ctx, item_index++, &item_bounds);
    DECODER selplat = decoder_by_id(app_configuration->platform);
    *showing_combo |= _vdec_combo(ctx, selplat);
    if (decoder_pref_requested != selplat)
    {
        nk_label_wrap(ctx, "Restart Moonlight to apply decoder change");
    }
    nk_layout_row_dynamic_s(ctx, 4, 1);
    return true;
}

static int _itemcount()
{
    return 1;
}

static void _windowopen()
{
}

static bool _navkey(struct nk_context *ctx, NAVKEY navkey, NAVKEY_STATE state, uint32_t timestamp)
{
    switch (navkey)
    {
    case NAVKEY_LEFT:
        if (settings_showing_combo)
        {
            return true;
        }
        else if (settings_hovered_item == 1)
        {
            if (state == NAVKEY_STATE_UP)
                settings_pane_item_offset(-1);
            return true;
        }
        return false;
    case NAVKEY_RIGHT:
        if (settings_showing_combo)
        {
            return true;
        }
        else if (settings_hovered_item == 0)
        {
            if (state == NAVKEY_STATE_UP)
                settings_pane_item_offset(1);
            return true;
        }
        return true;
    case NAVKEY_UP:
        if (settings_showing_combo)
        {
            return navkey_intercept_repeat(state, timestamp) || combo_item_select(-1);
        }
        else if (settings_hovered_item >= 2)
        {
            if (state == NAVKEY_STATE_UP)
                settings_pane_item_offset(0 - settings_hovered_item);
        }
        return true;
    case NAVKEY_DOWN:
        if (settings_showing_combo)
        {
            return navkey_intercept_repeat(state, timestamp) || combo_item_select(1);
        }
        else if (settings_hovered_item <= 1)
        {
            if (state == NAVKEY_STATE_UP)
                settings_pane_item_offset(2 - settings_hovered_item);
        }
        else
        {
            if (state == NAVKEY_STATE_UP)
                settings_pane_item_offset(1);
        }
        return true;
    case NAVKEY_CONFIRM:
        if (settings_showing_combo)
        {
            // Fake click on the item
            bus_pushevent(USER_FAKEINPUT_MOUSE_CLICK, &combo_focused_center, (void *)state);
            return true;
        }
    default:
        break;
    }
    return false;
}

bool _vdec_combo(struct nk_context *ctx, DECODER selplat)
{
    struct nk_rect combo_bounds = nk_widget_bounds(ctx);
    int combo_height = NK_MIN(200 * NK_UI_SCALE, ui_display_height - (combo_bounds.y + combo_bounds.h));
    if (nk_combo_begin_label(ctx, selplat > 0 ? decoder_definitions[selplat].name : "Automatic", nk_vec2(nk_widget_width(ctx), combo_height)))
    {
        if (combo_hovered_item.combo != 2)
        {
            combo_hovered_item.combo = 2;
            combo_hovered_item.count = 1 + decoder_orders_len;
            combo_hovered_item.request = -1;
            combo_hovered_item.item = -1;
        }
        nk_layout_row_dynamic_s(ctx, 25, 1);
        bool ever_hovered = false;
        for (int i = 0; i < 1 + decoder_orders_len; i++)
        {
            struct nk_rect ci_bounds = nk_widget_bounds(ctx);
            nk_bool hovered = nk_input_is_mouse_hovering_rect(&ctx->input, ci_bounds);
            if (hovered)
            {
                combo_hovered_item.item = i;
                ever_hovered = true;
            }
            if (combo_hovered_item.request == i)
            {
                // Send mouse pointer to the item
                combo_focused_center = nk_rect_center(ci_bounds);
                bus_pushevent(USER_FAKEINPUT_MOUSE_MOTION, &combo_focused_center, NULL);
                combo_hovered_item.request = -1;
            }
            if (i > 0)
            {
                MODULE_DEFINITION pdef = decoder_definitions[decoder_orders[i - 1]];
                if (nk_combo_item_label(ctx, pdef.name, NK_TEXT_LEFT))
                    app_configuration->platform = pdef.id;
            }
            else
            {
                if (nk_combo_item_label(ctx, "Automatic", NK_TEXT_LEFT))
                    app_configuration->platform = "auto";
            }
        }
        nk_combo_end(ctx);
        if (!ever_hovered)
        {
            combo_hovered_item.item = -1;
        }
        return true;
    }
    return false;
}

bool combo_item_select(int offset)
{
    if (combo_hovered_item.item < 0)
    {
        // No item focused before, select first one
        combo_hovered_item.request = 0;
    }
    else
    {
        int item = combo_hovered_item.item + offset;
        if (item >= 0 && item < combo_hovered_item.count)
        {
            combo_hovered_item.request = item;
        }
    }
    return true;
}

struct settings_pane settings_pane_adv_codec = {
    .title = "Decoder Settings",
    .render = _render,
    .navkey = _navkey,
    .itemcount = _itemcount,
    .onselect = NULL,
    .onwindowopen = _windowopen,
    .onwindowclose = NULL,
};
