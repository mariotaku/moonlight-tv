#include "window.h"
#include "priv.h"

#include "backend/computer_manager.h"

#include "util/user_event.h"
#include "util/bus.h"

static PSERVER_LIST pclist_focused_node;
static struct nk_vec2 pclist_focused_center;

static bool pclist_item_select(PSERVER_LIST list, int offset);

bool pclist_dropdown(struct nk_context *ctx, bool event_emitted)
{
    char *selected = selected_server_node != NULL ? selected_server_node->name : "Computer";
    nk_bool active;
    if (active = nk_combo_begin_label(ctx, selected, nk_vec2_s(200, 200)))
    {
        nk_layout_row_dynamic_s(ctx, 25, 1);
        PSERVER_LIST cur = computer_list;
        int i = 0;
        while (cur != NULL)
        {
            if (cur == pclist_focused_node)
            {
                // Send mouse pointer to the item
                struct nk_rect item_bounds = nk_widget_bounds(ctx);
                pclist_focused_center.x = nk_rect_center_x(item_bounds);
                pclist_focused_center.y = nk_rect_center_y(item_bounds);
                bus_pushevent(USER_FAKEINPUT_MOUSE_MOTION, &pclist_focused_center, NULL);
                pclist_focused_node = NULL;
            }
            if (nk_combo_item_label(ctx, cur->name, NK_TEXT_LEFT))
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
                        _open_pair(i, cur);
                    }
                    event_emitted = true;
                }
            }
            cur = cur->next;
            i++;
        }
        nk_combo_end(ctx);
    }
    return active || event_emitted;
}

bool pclist_dispatch_navkey(struct nk_context *ctx, NAVKEY key, bool down)
{
    switch (key)
    {
    case NAVKEY_BACK:
    case NAVKEY_MENU:
        // Fake touch on blank and cancel the combo
        nk_input_motion(ctx, 0, 0);
        nk_input_button(ctx, NK_BUTTON_LEFT, 0, 0, down);
        return true;
    case NAVKEY_UP:
        return down || pclist_item_select(computer_list, -1);
    case NAVKEY_DOWN:
        return down || pclist_item_select(computer_list, 1);
    case NAVKEY_CONFIRM:
        // Fake click on the item
        bus_pushevent(USER_FAKEINPUT_MOUSE_CLICK, &pclist_focused_center, (void *)down);
        return true;
    default:
        break;
    }
    return true;
}

bool pclist_item_select(PSERVER_LIST list, int offset)
{
    if (pclist_focused_node == NULL)
    {
        // No item focused before, select first one
        pclist_focused_node = list;
    }
    else
    {
        PSERVER_LIST item = serverlist_nth(pclist_focused_node, offset);
        if (item != NULL)
        {
            pclist_focused_node = item;
        }
    }
    return true;
}