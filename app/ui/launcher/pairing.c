#include "window.h"
#include "priv.h"

#include "app.h"

#include <stdlib.h>
#include <string.h>

#include "ui/root.h"
#include "ui/fonts.h"

#include "backend/computer_manager.h"

#include "util/bus.h"
#include "util/user_event.h"

#include "util/memlog.h"

static nk_bool nk_filter_ip(const struct nk_text_edit *box, nk_rune unicode);

static void manual_add_dismiss();
static void manual_add_callback(PPCMANAGER_RESP resp);

static struct nk_vec2 manual_add_cancel_center = {0, 0}, manual_add_confirm_center = {0, 0};

void _manual_add_window(struct nk_context *ctx)
{
    static char text[64];
    static int text_len;

    const char *message = "Enter the IP address of your GameStream PC:";
    struct nk_borders dec_size = nk_style_window_get_decoration_size(&ctx->style, NK_WINDOW_TITLE | NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR);
    int dialog_width = 330 * NK_UI_SCALE, message_width = dialog_width - dec_size.l - dec_size.r;
    static int message_height = 0;
    if (!message_height)
    {
        message_height = nk_text_wrap_measure_height(ctx, message_width, message, strlen(message)) + 1;
    }

    int dialog_height = dec_size.t + message_height +
                        ctx->style.window.spacing.y +
                        30 * NK_UI_SCALE + // Input bar
                        ctx->style.window.spacing.y +
                        5 * NK_UI_SCALE +
                        ctx->style.window.spacing.y +
                        30 * NK_UI_SCALE + // Buttons
                        dec_size.b;
    struct nk_rect s = nk_rect_centered(ui_display_width, ui_display_height, dialog_width, dialog_height);
    if (nk_begin(ctx, "Add PC Manually", s, NK_WINDOW_TITLE | NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR))
    {
        nk_layout_row_dynamic(ctx, message_height, 1);
        nk_label_wrap(ctx, message);
        nk_layout_row_dynamic_s(ctx, 30, 1);
        struct nk_rect editor_bounds = nk_widget_bounds(ctx);
        nk_flags editor_state = nk_edit_string(ctx, NK_EDIT_FIELD | NK_EDIT_GOTO_END_ON_ACTIVATE, text, &text_len, 64, nk_filter_ip);
        if (editor_state & NK_EDIT_ACTIVATED)
        {
            app_start_text_input(editor_bounds.x, editor_bounds.y, editor_bounds.w, editor_bounds.h);
        }
        else if (editor_state & NK_EDIT_DEACTIVATED)
        {
            app_stop_text_input();
        }

        nk_layout_row_dynamic_s(ctx, 5, 0);

        nk_layout_row_template_begin_s(ctx, 30);
        nk_layout_row_template_push_variable_s(ctx, 10);
        nk_layout_row_template_push_static_s(ctx, 80);
        nk_layout_row_template_push_static_s(ctx, 80);
        nk_layout_row_template_end(ctx);

        nk_spacing(ctx, 1);
        struct nk_rect btn_bounds = nk_widget_bounds(ctx);
        manual_add_cancel_center = nk_rect_center(btn_bounds);
        if (nk_button_label(ctx, "Cancel"))
        {
            // Clear input text
            text_len = 0;
            _launcher_show_manual_pair = false;
        }

        btn_bounds = nk_widget_bounds(ctx);
        manual_add_confirm_center = nk_rect_center(btn_bounds);
        if (nk_button_label(ctx, "OK") && text_len)
        {
            char *addr = malloc(text_len + 1);
            strncpy(addr, text, text_len);
            addr[text_len] = '\0';
            pcmanager_manual_add(addr, manual_add_callback);
            // Clear input text
            text_len = 0;
            _launcher_show_manual_pair = false;
        }
    }
    nk_end(ctx);
}

bool manual_add_navkey(struct nk_context *ctx, NAVKEY key, NAVKEY_STATE state, uint32_t timestamp)
{
    switch (key)
    {
    case NAVKEY_CANCEL:
        bus_pushevent(USER_FAKEINPUT_MOUSE_CLICK, &manual_add_cancel_center, (void *)state);
        return true;
    case NAVKEY_CONFIRM:
        bus_pushevent(USER_FAKEINPUT_MOUSE_CLICK, &manual_add_confirm_center, (void *)state);
        return true;
    }
    return true;
}

void manual_add_dismiss()
{
}

nk_bool nk_filter_ip(const struct nk_text_edit *box, nk_rune unicode)
{
    NK_UNUSED(box);
    if ((unicode < '0' || unicode > '9') &&
        (unicode < 'a' || unicode > 'f') &&
        (unicode < 'A' || unicode > 'F') &&
        unicode != ':' && unicode != '.')
        return nk_false;
    else
        return nk_true;
}

void _pairing_window(struct nk_context *ctx)
{
    const char *message = "Please enter pin below on your GameStream PC. This dialog will close when pairing is completed.";
    struct nk_borders dec_size = nk_style_window_get_decoration_size(&ctx->style, NK_WINDOW_TITLE | NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR);
    int dialog_width = 330 * NK_UI_SCALE, message_width = dialog_width - dec_size.l - dec_size.r;
    static int message_height = 0;
    if (!message_height)
    {
        message_height = nk_text_wrap_measure_height(ctx, message_width, message, strlen(message)) + 1;
    }
    int dialog_height = dec_size.t + message_height + ctx->style.window.spacing.y + 50 * NK_UI_SCALE + 12 * NK_UI_SCALE + dec_size.b;
    struct nk_rect s = nk_rect_centered(ui_display_width, ui_display_height, dialog_width, dialog_height);
    if (nk_begin(ctx, "Pairing", s, NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_NOT_INTERACTIVE | NK_WINDOW_NO_SCROLLBAR))
    {
        nk_layout_row_dynamic(ctx, message_height, 1);
        nk_label_wrap(ctx, message);
        nk_layout_row_dynamic_s(ctx, 50, 1);
        nk_style_push_font(ctx, &font_num_40->handle);
        nk_label(ctx, pairing_computer_state.pin, NK_TEXT_CENTERED);
        nk_style_pop_font(ctx);
    }
    nk_end(ctx);
}

void _unpairing_window(struct nk_context *ctx)
{
    struct nk_rect s = nk_rect_s_centered(ui_logic_width, ui_logic_height, 330, 60);
    if (nk_begin(ctx, "Unpairing", s, NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR))
    {
        struct nk_vec2 content_size = nk_window_get_content_inner_size(ctx);
        int content_height_remaining = (int)content_size.y;
        nk_layout_row_dynamic(ctx, content_height_remaining, 1);
        nk_label(ctx, "Unpairing...", NK_TEXT_ALIGN_LEFT);
    }
    nk_end(ctx);
}

void manual_add_callback(PPCMANAGER_RESP resp)
{
    if (resp->result.code != GS_OK)
        return;
    PSERVER_LIST node = serverlist_find_by(computer_list, resp->server->uuid, serverlist_compare_uuid);
    if (!node)
        return;
    if (node->server->paired)
        _select_computer(node, node->apps == NULL);
    else
        _open_pair(node);
}