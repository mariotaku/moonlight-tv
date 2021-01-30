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
#include "util/bus.h"
#include "util/user_event.h"

PSERVER_LIST selected_server_node;

struct nk_image launcher_default_cover;

struct pairing_computer_state pairing_computer_state;

static struct nk_style_button cm_list_button_style;
static struct nk_vec2 _computer_picker_center = {0, 0};

void _pairing_window(struct nk_context *ctx);
void _pairing_error_popup(struct nk_context *ctx);
void _server_error_popup(struct nk_context *ctx);
void _quitapp_window(struct nk_context *ctx);
void _quitapp_error_popup(struct nk_context *ctx);
void _webos_decoder_error_popup(struct nk_context *ctx);

bool pclist_dropdown(struct nk_context *ctx, bool event_emitted);
bool pclist_dispatch_navkey(struct nk_context *ctx, NAVKEY key, bool down);

bool _applist_dispatch_navkey(struct nk_context *ctx, PSERVER_LIST node, NAVKEY navkey, bool down, uint32_t timestamp);
void launcher_statbar(struct nk_context *ctx);

static void _launcher_modal_flags_update();
void _launcher_modal_popups_show(struct nk_context *ctx);
void _launcher_modal_windows_show(struct nk_context *ctx);

#define launcher_blocked() ((_launcher_modals & LAUNCHER_MODAL_MASK_WINDOW) || gui_settings_showing)

uint32_t _launcher_modals;
bool _launcher_showing_combo;
bool _launcher_popup_request_dismiss;
bool _quitapp_errno = false;

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
    _launcher_showing_combo = false;
    _launcher_modals = 0;
    if (launcher_blocked())
    {
        window_flags |= NK_WINDOW_NO_INPUT;
    }

    nk_style_push_vec2(ctx, &ctx->style.window.padding, nk_vec2_s(20, 15));
    if (nk_begin(ctx, "Moonlight", nk_rect(0, 0, gui_display_width, gui_display_height), window_flags))
    {
        int list_height = nk_window_get_content_inner_size(ctx).y;
        bool show_server_error_popup = false, show_pairing_error_popup = false,
             show_quitapp_error_popup = false;

        nk_style_push_float(ctx, &ctx->style.window.spacing.y, 10 * NK_UI_SCALE);
        bool event_emitted = false;
        nk_layout_row_template_begin_s(ctx, 25);
        nk_layout_row_template_push_static_s(ctx, 200);
        nk_layout_row_template_push_variable_s(ctx, 10);
        nk_layout_row_template_push_static_s(ctx, 25);
        nk_layout_row_template_end(ctx);
        list_height -= nk_widget_height(ctx);

        struct nk_rect bounds = nk_widget_bounds(ctx);
        _computer_picker_center.x = nk_rect_center_x(bounds);
        _computer_picker_center.y = nk_rect_center_y(bounds);
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
        }
        else
        {
            if (nk_group_begin(ctx, "launcher_empty", 0))
            {
                nk_layout_row_dynamic_s(ctx, 50, 1);
                nk_label(ctx, "Not selected", NK_TEXT_ALIGN_LEFT);
                nk_group_end(ctx);
            }
        }
        nk_style_pop_vec2(ctx);
        nk_style_pop_vec2(ctx);

        _launcher_modal_flags_update();
        launcher_statbar(ctx);

        _launcher_modal_popups_show(ctx);
    }
    nk_end(ctx);
    nk_style_pop_vec2(ctx);

    // Why Nuklear why, the button looks like "close" but it actually "hide"
    if (nk_window_is_hidden(ctx, "Moonlight"))
    {
        nk_window_close(ctx, "Moonlight");
        return false;
    }

    _launcher_modal_windows_show(ctx);
    // No popup, reset dismiss request
    if (!(_launcher_modals & LAUNCHER_MODAL_MASK_POPUP))
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
    case USER_CM_QUITAPP_FAILED:
    {
        _quitapp_errno = true;
        return true;
    }
    default:
        break;
    }
    return false;
}

bool launcher_window_dispatch_navkey(struct nk_context *ctx, NAVKEY key, bool down, uint32_t timestamp)
{
    bool key_handled = false;
    if (launcher_blocked())
    {
    }
    else if (_launcher_modals & LAUNCHER_MODAL_MASK_POPUP)
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
        key_handled |= _applist_dispatch_navkey(ctx, selected_server_node, key, down, timestamp);
    }
    if (key_handled)
    {
        return true;
    }
    switch (key)
    {
    case NAVKEY_MENU:
        if (_computer_picker_center.x && _computer_picker_center.y)
        {
            bus_pushevent(USER_FAKEINPUT_MOUSE_CLICK, &_computer_picker_center, (void *)down);
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

void _launcher_modal_flags_update()
{
    if (pairing_computer_state.state == PS_RUNNING)
    {
        _launcher_modals |= LAUNCHER_MODAL_PAIRING;
    }
    if (computer_manager_executing_quitapp)
    {
        _launcher_modals |= LAUNCHER_MODAL_QUITAPP;
    }
    if (_quitapp_errno)
    {
        _launcher_modals |= LAUNCHER_MODAL_QUITERR;
    }
    if (selected_server_node != NULL)
    {
        if (selected_server_node->server == NULL)
        {
            _launcher_modals |= LAUNCHER_MODAL_SERVERR;
        }
    }
    else
    {
        if (pairing_computer_state.state == PS_FAIL)
        {
            _launcher_modals |= LAUNCHER_MODAL_PAIRERR;
        }
    }
#if OS_WEBOS
    if (!app_webos_ndl && !app_webos_lgnc && !_webos_decoder_error_dismissed)
    {
        _launcher_modals |= LAUNCHER_MODAL_WDECERR;
    }
#endif
}
