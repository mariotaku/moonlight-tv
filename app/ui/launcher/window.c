#include "window.h"
#include "priv.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <memory.h>

#include "app.h"
#include "res.h"

#include "libgamestream/errors.h"

#include "backend/application_manager.h"
#include "ui/gui_root.h"
#include "ui/settings_window.h"

#if OS_WEBOS
#include "platform/webos/app_init.h"
#endif

#include "stream/input/absinput.h"
#include "util/user_event.h"

PSERVER_LIST selected_server_node;
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
static struct nk_rect _computer_picker_bounds = {0, 0, 0, 0};

static void _pairing_window(struct nk_context *ctx);
static void _pairing_error_popup(struct nk_context *ctx);
static void _server_error_popup(struct nk_context *ctx);
static void _quitapp_window(struct nk_context *ctx);
static void _webos_decoder_error_popup(struct nk_context *ctx);

bool pclist_dropdown(struct nk_context *ctx, bool event_emitted);
bool pclist_dispatch_navkey(struct nk_context *ctx, NAVKEY key, bool down);

bool _applist_dispatch_navkey(struct nk_context *ctx, PSERVER_LIST node, NAVKEY navkey, bool down);
void launcher_statbar(struct nk_context *ctx);

#define launcher_blocked() (pairing_computer_state.state == PS_RUNNING || gui_settings_showing)
bool _launcher_has_popup, _launcher_showing_combo;
bool _launcher_popup_request_dismiss;

void launcher_window_init(struct nk_context *ctx)
{
    launcher_default_cover = nk_image_id(0);
    if (!nk_imageloadm(res_default_cover_data, res_default_cover_size, &launcher_default_cover))
    {
        fprintf(stderr, "Cannot find assets/defcover.png\n");
        abort();
    }
    nk_image2texture(&launcher_default_cover);
    selected_server_node = NULL;
    pairing_computer_state.state = PS_NONE;
    memcpy(&cm_list_button_style, &(ctx->style.button), sizeof(struct nk_style_button));
    cm_list_button_style.text_alignment = NK_TEXT_ALIGN_LEFT;
}

void launcher_window_destroy()
{
    nk_imagetexturefree(&launcher_default_cover);
}


bool launcher_window(struct nk_context *ctx)
{
    int window_flags = NK_WINDOW_NO_SCROLLBAR;
    _launcher_has_popup = _launcher_showing_combo = false;
    if (launcher_blocked())
    {
        window_flags |= NK_WINDOW_NO_INPUT;
    }
    nk_style_push_vec2(ctx, &ctx->style.window.padding, nk_vec2_s(20, 15));
    if (nk_begin(ctx, "Moonlight", nk_rect(0, 0, gui_display_width, gui_display_height), window_flags))
    {
        int list_height = nk_window_get_content_inner_size(ctx).y;

        nk_style_push_float(ctx, &ctx->style.window.spacing.y, 10 * NK_UI_SCALE);
        bool event_emitted = false;
        nk_layout_row_template_begin_s(ctx, 25);
        nk_layout_row_template_push_static_s(ctx, 200);
        nk_layout_row_template_push_variable_s(ctx, 10);
        nk_layout_row_template_push_static_s(ctx, 25);
        nk_layout_row_template_end(ctx);
        list_height -= nk_widget_height(ctx);

        _computer_picker_bounds = nk_widget_bounds(ctx);
        _launcher_showing_combo = ctx->current->popup.win != NULL && ctx->current->popup.type != NK_PANEL_TOOLTIP;
        event_emitted |= pclist_dropdown(ctx, event_emitted);

        nk_spacing(ctx, 1);

        nk_style_push_vec2(ctx, &ctx->style.button.padding, nk_vec2_s(0, 0));

        if (nk_button_image(ctx, sprites_ui.ic_settings))
        {
            settings_window_open();
        }
        nk_style_pop_vec2(ctx);
        list_height -= ctx->style.window.spacing.y;

        nk_style_pop_float(ctx);

        list_height -= launcher_bottom_bar_height_dp * NK_UI_SCALE;
        list_height -= ctx->style.window.spacing.y;

        struct nk_list_view list_view;
        nk_layout_row_dynamic(ctx, list_height, 1);

        PSERVER_LIST selected = selected_server_node;

        nk_style_push_vec2(ctx, &ctx->style.window.group_padding, nk_vec2(0, 0));
        nk_style_push_vec2(ctx, &ctx->style.window.spacing, nk_vec2(10, 10));
        if (selected != NULL)
        {
            if (selected->server != NULL)
            {
                event_emitted |= launcher_applist(ctx, selected, event_emitted);
            }
            else
            {
                _server_error_popup(ctx);
                _launcher_has_popup |= true;
            }
        }
        else
        {
            if (nk_group_begin(ctx, "launcher_empty", 0))
            {
                nk_layout_row_dynamic_s(ctx, 50, 1);
                nk_label(ctx, "Not selected", NK_TEXT_ALIGN_LEFT);
                nk_group_end(ctx);
            }

            if (pairing_computer_state.state == PS_FAIL)
            {
                _pairing_error_popup(ctx);
                _launcher_has_popup |= true;
            }
        }
        nk_style_pop_vec2(ctx);
        nk_style_pop_vec2(ctx);

        launcher_statbar(ctx);

#ifdef OS_WEBOS
        if (!app_webos_ndl && !app_webos_lgnc && !_webos_decoder_error_dismissed)
        {
            _webos_decoder_error_popup(ctx);
            _launcher_has_popup |= true;
        }
#endif
    }
    nk_end(ctx);
    nk_style_pop_vec2(ctx);

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
    // No popup, reset dismiss request
    if (!_launcher_has_popup)
    {
        _launcher_popup_request_dismiss = false;
    }
    return true;
}

void launcher_display_size(struct nk_context *ctx, short width, short height)
{
}

bool launcher_window_dispatch_userevent(int which, void *data1, void *data2)
{
    switch (which)
    {
    case USER_CM_SERVER_ADDED:
    {
        // Select saved paired server if not selected before
        PSERVER_LIST node = data1;
        if (selected_server_node == NULL && node->server && node->server->paired &&
            app_configuration->address && strcmp(app_configuration->address, node->address) == 0)
        {
            _select_computer(node, node->apps == NULL);
        }
        return true;
    }
    default:
        break;
    }
    return false;
}

bool launcher_window_dispatch_navkey(struct nk_context *ctx, NAVKEY key, bool down)
{
    bool key_handled = false;
    if (launcher_blocked())
    {
    }
    else if (_launcher_has_popup)
    {
        if (!down && (key == NAVKEY_CANCEL || key == NAVKEY_START || key == NAVKEY_CONFIRM))
        {
            _launcher_popup_request_dismiss = true;
        }
    }
    else if (_launcher_showing_combo)
    {
        return pclist_dispatch_navkey(ctx, key, down);
    }
    else if (selected_server_node && selected_server_node->server)
    {
        key_handled |= _applist_dispatch_navkey(ctx, selected_server_node, key, down);
    }
    if (key_handled)
    {
        return true;
    }
    switch (key)
    {
    case NAVKEY_MENU:
        if (_computer_picker_bounds.w && _computer_picker_bounds.h)
        {
            int x = nk_rect_center_x(_computer_picker_bounds), y = nk_rect_center_y(_computer_picker_bounds);
            nk_input_motion(ctx, x, y);
            nk_input_button(ctx, NK_BUTTON_LEFT, x, y, down);
        }
        return true;
    case NAVKEY_CANCEL:
        if (!down)
        {
            request_exit();
        }
        return true;
    default:
        break;
    }
    return true;
}

void _select_computer(PSERVER_LIST node, bool load_apps)
{
    selected_server_node = node;
    pairing_computer_state.state = PS_NONE;
    app_configuration->address = node->address;
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

void _pairing_error_popup(struct nk_context *ctx)
{
    char *message = pairing_computer_state.error ? pairing_computer_state.error : "Pairing error.";
    enum nk_dialog_result result;
    if ((result = nk_dialog_popup_begin(ctx, "Pairing Failed", message, "OK", NULL, NULL)) != NK_DIALOG_NONE)
    {
        if (result != NK_DIALOG_RUNNING || _launcher_popup_request_dismiss)
        {
            pairing_computer_state.state = PS_NONE;
            nk_popup_close(ctx);
        }
        nk_popup_end(ctx);
    }
}

void _server_error_popup(struct nk_context *ctx)
{
    const char *message = selected_server_node->errmsg;
    enum nk_dialog_result result;
    if ((result = nk_dialog_popup_begin(ctx, "Connection Error", message, "OK", NULL, NULL)) != NK_DIALOG_NONE)
    {
        if (result != NK_DIALOG_RUNNING || _launcher_popup_request_dismiss)
        {
            selected_server_node = NULL;
            nk_popup_close(ctx);
        }
        nk_popup_end(ctx);
    }
}

void _webos_decoder_error_popup(struct nk_context *ctx)
{
    const char *message = "Unable to initialize system video decoder. "
                          "Audio and video will not work during streaming. "
                          "You may need to restart your TV.";
    enum nk_dialog_result result;
    if ((result = nk_dialog_popup_begin(ctx, "Decoder Error", message, "OK", NULL, NULL)) != NK_DIALOG_NONE)
    {
        if (result != NK_DIALOG_RUNNING || _launcher_popup_request_dismiss)
        {
            _webos_decoder_error_dismissed = true;
            nk_popup_close(ctx);
        }
        nk_popup_end(ctx);
    }
}