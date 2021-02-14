#include "window.h"

#include <stdlib.h>
#include <stdio.h>
static void _nk_edit_ushort(struct nk_context *ctx, unsigned short *num);

void _settings_pane_mouse(struct nk_context *ctx)
{
    nk_layout_row_dynamic_s(ctx, 25, 1);
    nk_label(ctx, "Absolute mouse mapping", NK_TEXT_LEFT);

    nk_layout_row_template_begin_s(ctx, 25);
    nk_layout_row_template_push_static_s(ctx, 120);
    nk_layout_row_template_push_dynamic(ctx);
    nk_layout_row_template_push_static_s(ctx, 10);
    nk_layout_row_template_push_dynamic(ctx);
    nk_layout_row_template_end(ctx);
    nk_label(ctx, "Desktop size", NK_TEXT_LEFT);
    _nk_edit_ushort(ctx, &app_configuration->absmouse_mapping.desktop_w);
    nk_label(ctx, "*", NK_TEXT_CENTERED);
    _nk_edit_ushort(ctx, &app_configuration->absmouse_mapping.desktop_h);

    nk_layout_row_template_begin_s(ctx, 25);
    nk_layout_row_template_push_static_s(ctx, 120);
    nk_layout_row_template_push_dynamic(ctx);
    nk_layout_row_template_push_static_s(ctx, 10);
    nk_layout_row_template_push_dynamic(ctx);
    nk_layout_row_template_end(ctx);
    nk_label(ctx, "Screen size", NK_TEXT_LEFT);
    _nk_edit_ushort(ctx, &app_configuration->absmouse_mapping.screen_w);
    nk_label(ctx, "*", NK_TEXT_CENTERED);
    _nk_edit_ushort(ctx, &app_configuration->absmouse_mapping.screen_h);

    nk_layout_row_template_begin_s(ctx, 25);
    nk_layout_row_template_push_static_s(ctx, 120);
    nk_layout_row_template_push_dynamic(ctx);
    nk_layout_row_template_push_static_s(ctx, 10);
    nk_layout_row_template_push_dynamic(ctx);
    nk_layout_row_template_end(ctx);
    nk_label(ctx, "Screen position", NK_TEXT_LEFT);
    _nk_edit_ushort(ctx, &app_configuration->absmouse_mapping.screen_x);
    nk_label(ctx, ",", NK_TEXT_CENTERED);
    _nk_edit_ushort(ctx, &app_configuration->absmouse_mapping.screen_y);
}

static nk_bool nk_filter_numeric(const struct nk_text_edit *box, nk_rune unicode)
{
    NK_UNUSED(box);
    if (unicode < '0' || unicode > '9')
        return nk_false;
    else
        return nk_true;
}

void _nk_edit_ushort(struct nk_context *ctx, unsigned short *num)
{
    static char buf[8];
    snprintf(buf, 7, "%d", *num);
    nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, buf, 5, nk_filter_numeric);
    *num = atoi(buf);
}