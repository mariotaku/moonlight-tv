#include <stdio.h>
#include <stdbool.h>
#include <memory.h>

#include "libgamestream/errors.h"

#include "backend/application_manager.h"
#include "computers_window.h"
#include "applications_window.h"

#define LINKEDLIST_TYPE PSERVER_LIST
#include "util/linked_list.h"

static int selected_computer_idx;

typedef enum pairing_state
{
    PS_NONE,
    PS_RUNNING,
    PS_FAIL
} pairing_state;

static struct
{
    pairing_state state;
    char pin[5];
    char *error;
} pairing_computer_state;

static struct nk_style_button cm_list_button_style;

static void pairing_window(struct nk_context *ctx);
static void pairing_error_dialog(struct nk_context *ctx);
static void cw_open_computer(int index, PSERVER_LIST node);
static void cw_open_pair(int index, SERVER_DATA *item);

void computers_window_init(struct nk_context *ctx)
{
    selected_computer_idx = -1;
    pairing_computer_state.state = PS_NONE;
    memcpy(&cm_list_button_style, &(ctx->style.button), sizeof(struct nk_style_button));
    cm_list_button_style.text_alignment = NK_TEXT_ALIGN_LEFT;
}

bool computers_window(struct nk_context *ctx)
{
    /* GUI */
    nk_flags computers_window_flags = NK_WINDOW_BORDER | NK_WINDOW_CLOSABLE | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_TITLE;
    if (selected_computer_idx != -1 || pairing_computer_state.state != PS_NONE)
    {
        computers_window_flags |= NK_WINDOW_NO_INPUT;
    }
    int content_height_remaining;
    if (nk_begin(ctx, "Computers", nk_rect(50, 50, 300, 300), computers_window_flags))
    {
        content_height_remaining = (int)nk_window_get_content_region_size(ctx).y;
        content_height_remaining -= ctx->style.window.padding.y * 2;
        content_height_remaining -= (int)ctx->style.window.border;
        nk_menubar_begin(ctx);
        content_height_remaining -= 25;
        nk_layout_row_begin(ctx, NK_STATIC, 25, 2);
        nk_layout_row_push(ctx, 45);
        if (nk_menu_begin_label(ctx, "FILE", NK_TEXT_LEFT, nk_vec2(120, 200)))
        {
            nk_layout_row_dynamic(ctx, 30, 1);
            nk_menu_item_label(ctx, "OPEN", NK_TEXT_LEFT);
            nk_menu_item_label(ctx, "CLOSE", NK_TEXT_LEFT);
            nk_menu_end(ctx);
        }
        nk_layout_row_push(ctx, 45);
        if (nk_menu_begin_label(ctx, "EDIT", NK_TEXT_LEFT, nk_vec2(120, 200)))
        {
            nk_layout_row_dynamic(ctx, 30, 1);
            nk_menu_item_label(ctx, "COPY", NK_TEXT_LEFT);
            nk_menu_item_label(ctx, "CUT", NK_TEXT_LEFT);
            nk_menu_item_label(ctx, "PASTE", NK_TEXT_LEFT);
            nk_menu_end(ctx);
        }
        nk_layout_row_end(ctx);
        nk_menubar_end(ctx);

        struct nk_list_view list_view;
        PSERVER_LIST computer_list = computer_manager_list();
        int computer_len = linkedlist_len(computer_list);
        nk_layout_row_dynamic(ctx, content_height_remaining, 1);
        if (nk_list_view_begin(ctx, &list_view, "computers_list", NK_WINDOW_BORDER, 25, computer_len))
        {
            nk_layout_row_dynamic(ctx, 25, 1);
            PSERVER_LIST cur = linkedlist_nth(computer_list, list_view.begin);

            for (int i = 0; i < list_view.count; ++i, cur = cur->next)
            {
                SERVER_DATA *item = (SERVER_DATA *)cur->server;
                if (nk_widget_is_mouse_clicked(ctx, NK_BUTTON_LEFT))
                {
                    if (item->paired)
                    {
                        cw_open_computer(i, cur);
                    }
                    else
                    {
                        cw_open_pair(i, item);
                    }
                }
                nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "%s%s", item->paired ? "" : "[+] ", item->serverInfo.address);
            }
            nk_list_view_end(&list_view);
        }
    }
    nk_end(ctx);

    // Why Nuklear why, the button looks like "close" but it actually "hide"
    if (nk_window_is_hidden(ctx, "Computers"))
    {
        nk_window_close(ctx, "Computers");
        return false;
    }

    if (selected_computer_idx != -1)
    {
        PSERVER_LIST node = computer_manager_server_at(selected_computer_idx);
        if (!applications_window(ctx, node))
        {
            selected_computer_idx = -1;
        }
    }
    else if (pairing_computer_state.state == PS_RUNNING)
    {
        pairing_window(ctx);
    }
    else if (pairing_computer_state.state == PS_FAIL)
    {
        pairing_error_dialog(ctx);
    }
    return true;
}

void cw_open_computer(int index, PSERVER_LIST node)
{
    selected_computer_idx = index;
    pairing_computer_state.state = PS_NONE;
    if (node->apps == NULL)
    {
        application_manager_load(node);
    }
}

static void cw_pairing_callback(int result, const char *error)
{
    if (result == GS_OK)
    {
        // Close pairing window
        pairing_computer_state.state = PS_NONE;
    }
    else
    {
        // Show pairing error instead
        pairing_computer_state.state = PS_FAIL;
        pairing_computer_state.error = (char *)error;
    }
}

void cw_open_pair(int index, SERVER_DATA *item)
{
    selected_computer_idx = -1;
    pairing_computer_state.state = PS_RUNNING;
    computer_manager_pair(item, &pairing_computer_state.pin[0], cw_pairing_callback);
}

void pairing_window(struct nk_context *ctx)
{
    if (nk_begin(ctx, "Pairing", nk_rect(350, 150, 300, 100),
                 NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR))
    {
        nk_layout_row_dynamic(ctx, 50, 1);

        nk_labelf_wrap(ctx, "Please enter %s on your GameStream PC. This dialog will close when pairing is completed.",
                       pairing_computer_state.pin);
    }
    nk_end(ctx);
}

void pairing_error_dialog(struct nk_context *ctx)
{
    struct nk_vec2 win_region_size;
    int content_height_remaining;
    if (nk_begin(ctx, "Pairing Failed", nk_rect(350, 150, 300, 100),
                 NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR))
    {
        win_region_size = nk_window_get_content_region_size(ctx);
        content_height_remaining = (int)win_region_size.y;
        content_height_remaining -= 8 * 2;
        /* remove bottom button height */
        content_height_remaining -= 30;
        nk_layout_row_dynamic(ctx, content_height_remaining, 1);

        if (pairing_computer_state.error != NULL)
        {
            nk_label_wrap(ctx, pairing_computer_state.error);
        }
        else
        {
            nk_label_wrap(ctx, "Pairing error.");
        }
        nk_layout_space_begin(ctx, NK_STATIC, 30, 1);
        nk_layout_space_push(ctx, nk_recti(win_region_size.x - 80, 0, 80, 30));
        if (nk_button_label(ctx, "OK"))
        {
            pairing_computer_state.state = PS_NONE;
        }
        nk_layout_space_end(ctx);
    }
    nk_end(ctx);
}