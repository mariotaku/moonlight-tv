#include "window.h"
#include "priv.h"

#include <string.h>

#include "ui/root.h"
#include "ui/fonts.h"
#include "backend/computer_manager.h"

bool _decoder_error_dismissed;

void _manual_add_window(struct nk_context *ctx);
bool manual_add_navkey(struct nk_context *ctx, NAVKEY key, NAVKEY_STATE state, uint32_t timestamp);

void _pairing_window(struct nk_context *ctx);
void _unpairing_window(struct nk_context *ctx);

void _quitapp_window(struct nk_context *ctx)
{
    struct nk_rect s = nk_rect_s_centered(ui_logic_width, ui_logic_height, 330, 60);
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
    const char *message = pairing_computer_state.error ? pairing_computer_state.error : "Pairing error.";
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
    const char *message;
    if (selected_server_node && selected_server_node->state.code == SERVER_STATE_ERROR && selected_server_node->state.error.errmsg)
        message = selected_server_node->state.error.errmsg;
    else
        message = "";

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

void _quitapp_error_popup(struct nk_context *ctx)
{
    const char *message = "Unable to quit game. Please quit the game on the device you started the session.";
    enum nk_dialog_result result;
    if ((result = nk_dialog_popup_begin(ctx, "Quit Game", message, "OK", NULL, NULL)) != NK_DIALOG_NONE)
    {
        if (result != NK_DIALOG_RUNNING || _launcher_popup_request_dismiss)
        {
            _quitapp_errno = false;
            nk_popup_close(ctx);
        }
        nk_popup_end(ctx);
    }
}

void _decoder_warning_popup(struct nk_context *ctx)
{
    const char *message = "No functioning hardware accelerated video decoder was detected.\n"
                          "Your streaming performance may be severely degraded in this configuration.";
    enum nk_dialog_result result;
    if ((result = nk_dialog_popup_begin(ctx, "No Hardware Decoder", message, "OK", NULL, NULL)) != NK_DIALOG_NONE)
    {
        if (result != NK_DIALOG_RUNNING || _launcher_popup_request_dismiss)
        {
            _decoder_error_dismissed = true;
            nk_popup_close(ctx);
        }
        nk_popup_end(ctx);
    }
}

void _hostinfo_popup(struct nk_context *ctx)
{
    enum nk_dialog_result result;
    char message[1024];
    if (selected_server_node)
    {
        const SERVER_DATA *server = selected_server_node->server;
        int index = snprintf(message, sizeof(message), "Name: %s", server->hostname);
        if (selected_server_node->server)
        {
            index += snprintf(&message[index], sizeof(message) - index, "\nIP Address: %s", server->serverInfo.address);
        }
        index += snprintf(&message[index], sizeof(message) - index, "\nUUID: %s", server->uuid);
        index += snprintf(&message[index], sizeof(message) - index, "\nMAC Address: %s", server->mac);
    }
    else
    {
        snprintf(message, sizeof(message), "Unknown host");
    }
    //selected_server_node
    if ((result = nk_dialog_popup_begin(ctx, "Host Details", message, "OK", NULL, "Unbind")) != NK_DIALOG_NONE)
    {
        if (result != NK_DIALOG_RUNNING || _launcher_popup_request_dismiss)
        {
            _launcher_show_host_info = false;
            if (result == NK_DIALOG_NEUTRAL)
            {
                _open_unpair(selected_server_node);
            }
            nk_popup_close(ctx);
        }
        nk_popup_end(ctx);
    }
}

void _launcher_modal_popups_show(struct nk_context *ctx)
{
    if (_launcher_modals & LAUNCHER_MODAL_NOHWCODEC)
    {
        _decoder_warning_popup(ctx);
    }
    else if (_launcher_modals & LAUNCHER_MODAL_SERVERR)
    {
        _server_error_popup(ctx);
    }
    else if (_launcher_modals & LAUNCHER_MODAL_PAIRERR)
    {
        _pairing_error_popup(ctx);
    }
    else if (_launcher_modals & LAUNCHER_MODAL_QUITERR)
    {
        _quitapp_error_popup(ctx);
    }
    else if (_launcher_modals & LAUNCHER_MODAL_HOSTINFO)
    {
        _hostinfo_popup(ctx);
    }
}

void _launcher_modal_windows_show(struct nk_context *ctx)
{
    if (_launcher_modals & LAUNCHER_MODAL_MANUAL_ADD)
    {
        _manual_add_window(ctx);
    }
    if (_launcher_modals & LAUNCHER_MODAL_PAIRING)
    {
        _pairing_window(ctx);
    }
    else if (_launcher_modals & LAUNCHER_MODAL_UNPAIRING)
    {
        _unpairing_window(ctx);
    }
    else if (_launcher_modals & LAUNCHER_MODAL_QUITAPP)
    {
        _quitapp_window(ctx);
    }
}

bool _launcher_modal_windows_navkey(struct nk_context *ctx, NAVKEY key, NAVKEY_STATE state, uint32_t timestamp)
{
    if (_launcher_modals & LAUNCHER_MODAL_MANUAL_ADD)
    {
        return manual_add_navkey(ctx, key, state, timestamp);
    }
    return false;
}