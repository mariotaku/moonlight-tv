#include "launcher_window.h"

#include <stdio.h>
#include <stdbool.h>
#include <memory.h>

#include "libgamestream/errors.h"

#include "backend/application_manager.h"
#include "gui_root.h"
#include "settings_window.h"

#if OS_WEBOS
#include "platform/webos/app_init.h"
#endif

#define LINKEDLIST_TYPE PSERVER_LIST
#include "util/linked_list.h"

#include "res.h"

static PSERVER_LIST selected_server_node;
static bool _webos_decoder_error_dismissed;

struct nk_image launcher_default_cover;
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

static void _select_computer(PSERVER_LIST node, bool load_apps);
static void _open_pair(int index, PSERVER_LIST node);

static void _pairing_window(struct nk_context *ctx);
static void _pairing_error_popup(struct nk_context *ctx);
static void _server_error_popup(struct nk_context *ctx);
static void _quitapp_window(struct nk_context *ctx);
static void _webos_decoder_error_popup(struct nk_context *ctx);

static bool cw_computer_dropdown(struct nk_context *ctx, PSERVER_LIST list, bool event_emitted);

void launcher_window_init(struct nk_context *ctx)
{
    launcher_default_cover = nk_image_id(0);
    if (!nk_loadimgmem(res_default_cover_data, res_default_cover_size, &launcher_default_cover))
    {
        fprintf(stderr, "Cannot find assets/defcover.png\n");
        abort();
    }
    nk_conv2gl(&launcher_default_cover);
    selected_server_node = NULL;
    pairing_computer_state.state = PS_NONE;
    memcpy(&cm_list_button_style, &(ctx->style.button), sizeof(struct nk_style_button));
    cm_list_button_style.text_alignment = NK_TEXT_ALIGN_LEFT;
}

void launcher_window_destroy()
{
    nk_freeimage(&launcher_default_cover);
}

bool launcher_window(struct nk_context *ctx)
{
    int window_flags = NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER;
    if (pairing_computer_state.state == PS_RUNNING || gui_settings_showing)
    {
        window_flags |= NK_WINDOW_NO_INPUT;
    }
    if (nk_begin(ctx, "Moonlight", nk_rect_s(20, 20, gui_logic_width - 40, gui_logic_height - 40),
                 window_flags))
    {
        struct nk_vec2 list_size = nk_window_get_content_inner_size(ctx);

        bool event_emitted = false;
        nk_layout_row_template_begin_s(ctx, 25);
        nk_layout_row_template_push_static_s(ctx, 200);
        nk_layout_row_template_push_variable_s(ctx, 10);
        nk_layout_row_template_push_static_s(ctx, 80);
        nk_layout_row_template_end(ctx);
        list_size.y -= nk_widget_height(ctx);
        list_size.y -= ctx->style.window.spacing.y;

        int computer_len = linkedlist_len(computer_list);
        event_emitted |= cw_computer_dropdown(ctx, computer_list, event_emitted);

        nk_spacing(ctx, 1);

        if (nk_button_label(ctx, "Settings"))
        {
            settings_window_open();
        }

        struct nk_list_view list_view;
        nk_layout_row_dynamic(ctx, list_size.y, 1);
        PSERVER_LIST selected = selected_server_node;

        if (selected != NULL)
        {
            if (selected->server != NULL)
            {
                event_emitted |= launcher_applist(ctx, selected, event_emitted);
            }
            else
            {
                _server_error_popup(ctx);
            }
        }
        else
        {
            if (nk_group_begin(ctx, "launcher_empty", NK_WINDOW_BORDER))
            {
                nk_layout_row_dynamic_s(ctx, 50, 1);
                nk_label(ctx, "Not selected", NK_TEXT_ALIGN_LEFT);
                nk_group_end(ctx);
            }

            if (pairing_computer_state.state == PS_FAIL)
            {
                _pairing_error_popup(ctx);
            }
        }
#ifdef OS_WEBOS
        if (!app_webos_ndl && !app_webos_lgnc && !_webos_decoder_error_dismissed)
        {
            _webos_decoder_error_popup(ctx);
        }
#endif
    }
    nk_end(ctx);

    if (pairing_computer_state.state == PS_RUNNING)
    {
        _pairing_window(ctx);
    }
    else if (computer_manager_executing_quitapp)
    {
        _quitapp_window(ctx);
    }

    // Why Nuklear why, the button looks like "close" but it actually "hide"
    if (nk_window_is_hidden(ctx, "Moonlight"))
    {
        nk_window_close(ctx, "Moonlight");
        return false;
    }
    return true;
}

void launcher_display_size(struct nk_context *ctx, short width, short height)
{
}

bool cw_computer_dropdown(struct nk_context *ctx, PSERVER_LIST list, bool event_emitted)
{
    char *selected = selected_server_node != NULL ? selected_server_node->name : "Computer";
    bool active = ctx->current->popup.win != NULL && ctx->current->popup.type != NK_PANEL_TOOLTIP;
    if (nk_combo_begin_label(ctx, selected, nk_vec2_s(200, 200)))
    {
        nk_layout_row_dynamic_s(ctx, 25, 1);
        PSERVER_LIST cur = list;
        int i = 0;
        while (cur != NULL)
        {
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
        if (nk_combo_item_label(ctx, computer_discovery_running ? "Scanning..." : "Rescan", NK_TEXT_LEFT))
        {
            if (!event_emitted)
            {
                computer_manager_polling_start();
            }
            event_emitted = true;
        }
        nk_combo_end(ctx);
    }
    return active || event_emitted;
}

void _select_computer(PSERVER_LIST node, bool load_apps)
{
    selected_server_node = node;
    pairing_computer_state.state = PS_NONE;
    if (load_apps)
    {
        application_manager_load(node);
    }
}

static void cw_pairing_callback(PSERVER_LIST node, int result, const char *error)
{
    if (result == GS_OK)
    {
        // Close pairing window
        pairing_computer_state.state = PS_NONE;
        _select_computer(node, node->apps == NULL);
    }
    else
    {
        // Show pairing error instead
        pairing_computer_state.state = PS_FAIL;
        pairing_computer_state.error = (char *)error;
    }
}

void _open_pair(int index, PSERVER_LIST node)
{
    selected_server_node = NULL;
    pairing_computer_state.state = PS_RUNNING;
    computer_manager_pair(node, &pairing_computer_state.pin[0], cw_pairing_callback);
}

void _pairing_window(struct nk_context *ctx)
{
    struct nk_rect s = nk_rect_s_centered(gui_logic_width, gui_logic_height, 330, 110);
    if (nk_begin(ctx, "Pairing", s, NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_NOT_INTERACTIVE | NK_WINDOW_NO_SCROLLBAR))
    {
        nk_layout_row_dynamic_s(ctx, 64, 1);

        nk_labelf_wrap(ctx, "Please enter %s on your GameStream PC. This dialog will close when pairing is completed.",
                       pairing_computer_state.pin);
    }
    nk_end(ctx);
}

void _pairing_error_popup(struct nk_context *ctx)
{
    struct nk_vec2 window_size = nk_window_get_size(ctx);
    struct nk_rect s = nk_rect_centered(window_size.x, window_size.y, 300 * NK_UI_SCALE, 110 * NK_UI_SCALE);
    if (nk_popup_begin(ctx, NK_POPUP_STATIC, "Pairing Failed",
                       NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR, s))
    {

        struct nk_vec2 content_size = nk_window_get_content_inner_size(ctx);
        int content_height_remaining = (int)content_size.y;
        /* remove bottom button height */
        content_height_remaining -= 30 * NK_UI_SCALE;
        content_height_remaining -= ctx->style.window.spacing.y;
        nk_layout_row_dynamic(ctx, content_height_remaining, 1);

        if (pairing_computer_state.error != NULL)
        {
            nk_label_wrap(ctx, pairing_computer_state.error);
        }
        else
        {
            nk_label_wrap(ctx, "Pairing error.");
        }

        nk_layout_row_template_begin_s(ctx, 30);
        nk_layout_row_template_push_variable_s(ctx, 10);
        nk_layout_row_template_push_static_s(ctx, 80);
        nk_layout_row_template_end(ctx);

        nk_spacing(ctx, 1);
        if (nk_button_label(ctx, "OK"))
        {
            pairing_computer_state.state = PS_NONE;
            nk_popup_close(ctx);
        }
        nk_popup_end(ctx);
    }
}

void _server_error_popup(struct nk_context *ctx)
{
    struct nk_vec2 window_size = nk_window_get_size(ctx);
    struct nk_rect s = nk_rect_centered(window_size.x, window_size.y, 300 * NK_UI_SCALE, 110 * NK_UI_SCALE);
    if (nk_popup_begin(ctx, NK_POPUP_STATIC, "Connection Error",
                       NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR, s))
    {
        struct nk_vec2 content_size = nk_window_get_content_inner_size(ctx);
        int content_height_remaining = (int)content_size.y;
        /* remove bottom button height */
        content_height_remaining -= 30 * NK_UI_SCALE;
        content_height_remaining -= ctx->style.window.spacing.y;
        nk_layout_row_dynamic(ctx, content_height_remaining, 1);
        nk_label_wrap(ctx, selected_server_node->errmsg);

        nk_layout_row_template_begin_s(ctx, 30);
        nk_layout_row_template_push_variable_s(ctx, 10);
        nk_layout_row_template_push_static_s(ctx, 80);
        nk_layout_row_template_end(ctx);

        nk_spacing(ctx, 1);
        if (nk_button_label(ctx, "OK"))
        {
            selected_server_node = NULL;
            nk_popup_close(ctx);
        }
        nk_popup_end(ctx);
    }
}

void _quitapp_window(struct nk_context *ctx)
{
    struct nk_rect s = nk_rect_s_centered(gui_logic_width, gui_logic_height, 330, 60);
    if (nk_begin(ctx, "Connection", s, NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR))
    {
        struct nk_vec2 content_size = nk_window_get_content_inner_size(ctx);
        int content_height_remaining = (int)content_size.y;
        nk_layout_row_dynamic(ctx, content_height_remaining, 1);

        nk_label(ctx, "Quitting...", NK_TEXT_ALIGN_LEFT);
    }
    nk_end(ctx);
}

void _webos_decoder_error_popup(struct nk_context *ctx)
{
    struct nk_vec2 window_size = nk_window_get_size(ctx);
    struct nk_rect s = nk_rect_centered(window_size.x, window_size.y, 300 * NK_UI_SCALE, 176 * NK_UI_SCALE);
    if (nk_popup_begin(ctx, NK_POPUP_STATIC, "Decoder Error",
                       NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR, s))
    {
        struct nk_vec2 content_size = nk_window_get_content_inner_size(ctx);
        int content_height_remaining = (int)content_size.y;
        /* remove bottom button height */
        content_height_remaining -= 30 * NK_UI_SCALE;
        content_height_remaining -= ctx->style.window.spacing.y;
        nk_layout_row_dynamic(ctx, content_height_remaining, 1);
        nk_label_wrap(ctx, "Unable to initialize system video decoder. Audio and video will not work during streaming. You may need to restart your TV.");

        nk_layout_row_template_begin_s(ctx, 30);
        nk_layout_row_template_push_variable_s(ctx, 10);
        nk_layout_row_template_push_static_s(ctx, 80);
        nk_layout_row_template_end(ctx);

        nk_spacing(ctx, 1);
        if (nk_button_label(ctx, "OK"))
        {
            _webos_decoder_error_dismissed = true;
            nk_popup_close(ctx);
        }
        nk_popup_end(ctx);
    }
}