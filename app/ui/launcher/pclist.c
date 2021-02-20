#include "window.h"
#include "priv.h"

#include "backend/computer_manager.h"

#include "util/user_event.h"
#include "util/bus.h"

static PSERVER_LIST pclist_hovered_item = NULL, pclist_hover_request = NULL;
static struct nk_vec2 pclist_focused_center;

static bool pclist_item_select(PSERVER_LIST list, int offset);

bool pclist_showing;

bool pclist_dropdown(struct nk_context *ctx, bool event_emitted)
{
    char *selected = selected_server_node != NULL ? selected_server_node->hostname : "Computer";
    nk_style_push_vec2(ctx, &ctx->style.window.popup_padding, nk_vec2_s(0, 5));
    if ((pclist_showing = nk_combo_begin_label(ctx, selected, nk_vec2_s(200, 200))))
    {
        nk_layout_row_dynamic_s(ctx, 25, 1);
        int i = 0;
        for (PSERVER_LIST cur = computer_list; cur != NULL; cur = cur->next, i++)
        {
            struct nk_rect item_bounds = nk_widget_bounds(ctx);
            nk_bool hovered = nk_input_is_mouse_hovering_rect(&ctx->input, item_bounds);
            if (hovered)
            {
                pclist_hovered_item = cur;
            }
            if (cur == pclist_hover_request)
            {
                // Send mouse pointer to the item
                pclist_focused_center = nk_rect_center(item_bounds);
                bus_pushevent(USER_FAKEINPUT_MOUSE_MOTION, &pclist_focused_center, NULL);
                pclist_hover_request = NULL;
            }
            if (nk_combo_item_label(ctx, cur->hostname, NK_TEXT_LEFT))
            {
                if (!event_emitted)
                {
                    SERVER_DATA *server = (SERVER_DATA *)cur->server;
                    if (server == NULL)
                    {
                        _select_computer(cur, false);
                    }
                    else if (server->paired)
                    {
                        _select_computer(cur, cur->apps == NULL);
                    }
                    else
                    {
                        _open_pair(cur);
                    }
                    event_emitted = true;
                }
            }
        }
        nk_combo_end(ctx);
    }
    nk_style_pop_vec2(ctx);
    return pclist_showing || event_emitted;
}

bool pclist_dispatch_navkey(struct nk_context *ctx, NAVKEY key, NAVKEY_STATE state, uint32_t timestamp)
{
    switch (key)
    {
    case NAVKEY_CANCEL:
    case NAVKEY_MENU:
        // Fake touch on blank and cancel the combo
        nk_input_motion(ctx, 0, 0);
        nk_input_button(ctx, NK_BUTTON_LEFT, 0, 0, state);
        return true;
    case NAVKEY_UP:
        return navkey_intercept_repeat(state, timestamp) || pclist_item_select(computer_list, -1);
    case NAVKEY_DOWN:
        return navkey_intercept_repeat(state, timestamp) || pclist_item_select(computer_list, 1);
    case NAVKEY_START:
    case NAVKEY_CONFIRM:
        if (pclist_hovered_item)
        {
            // Fake click on the item
            bus_pushevent(USER_FAKEINPUT_MOUSE_CLICK, &pclist_focused_center, (void *)state);
        }
        return true;
    default:
        break;
    }
    return true;
}

bool pclist_item_select(PSERVER_LIST list, int offset)
{
    if (pclist_hover_request == NULL)
    {
        // No item focused before, select first one
        pclist_hover_request = list;
    }
    else
    {
        PSERVER_LIST item = serverlist_nth(pclist_hover_request, offset);
        if (item != NULL)
        {
            pclist_hover_request = item;
        }
    }
    return true;
}