#pragma once
#ifndef NK_NUKLEAR_H_
#include "nuklear.h"
#include "ext_sprites.h"
#endif

#ifndef NK_UI_SCALE
#warning "NK_UI_SCALE not defined"
#define NK_UI_SCALE 1
#endif

enum nk_ext_style_colors
{
    NK_EXT_COLOR_FOCUSED,
    NK_EXT_COLOR_FOCUSED_PRESSED,
    NK_EXT_COLOR_COUNT
};

extern const struct nk_color nk_ext_color_style[NK_EXT_COLOR_COUNT];

void nk_ext_apply_style(struct nk_context *ctx);

#ifdef NK_IMPLEMENTATION

const struct nk_color nk_ext_color_style[NK_EXT_COLOR_COUNT] = {
#define NK_COLOR(n, r, g, b, a) {r, g, b, a},
    // From https://moonlight-stream.org/ link highlight
    NK_COLOR(NK_EXT_COLOR_FOCUSED, 176, 188, 223, 255) 
    NK_COLOR(NK_EXT_COLOR_FOCUSED_PRESSED, 142, 152, 182, 255) 
#undef NK_COLOR
};

void nk_ext_apply_style(struct nk_context *ctx)
{

    struct nk_color color_table[NK_COLOR_COUNT] = {
#define NK_COLOR(n, r, g, b, a) {r, g, b, a},
        NK_COLOR_MAP(NK_COLOR)
#undef NK_COLOR
    };
    color_table[NK_COLOR_TEXT] = nk_rgb(216, 216, 216);
    color_table[NK_COLOR_EDIT_CURSOR] = nk_rgb(216, 216, 216);

    nk_style_from_table(ctx, color_table);

    struct nk_style *style;
    struct nk_style_text *text;
    struct nk_style_button *button;
    struct nk_style_toggle *toggle;
    struct nk_style_selectable *select;
    struct nk_style_slider *slider;
    struct nk_style_progress *prog;
    struct nk_style_scrollbar *scroll;
    struct nk_style_edit *edit;
    struct nk_style_property *property;
    struct nk_style_combo *combo;
    struct nk_style_chart *chart;
    struct nk_style_tab *tab;
    struct nk_style_window *win;

    NK_ASSERT(ctx);
    if (!ctx)
        return;
    style = &ctx->style;

    /* default button */
    button = &style->button;
    button->padding = nk_vec2_s(2.0f, 2.0f);
    button->border = 1.0f * NK_UI_SCALE;
    button->rounding = 4.0f * NK_UI_SCALE;

    /* contextual button */
    button = &style->contextual_button;
    button->padding = nk_vec2_s(2.0f, 2.0f);

    /* menu button */
    button = &style->menu_button;
    button->padding = nk_vec2_s(2.0f, 2.0f);
    button->border = 0.0f * NK_UI_SCALE;
    button->rounding = 1.0f * NK_UI_SCALE;

    /* checkbox toggle */
    toggle = &style->checkbox;
    toggle->normal = nk_style_item_image(sprites_ui.ic_check_box_blank);
    toggle->hover = nk_style_item_image(sprites_ui.ic_check_box_blank_hovered);
    toggle->active = nk_style_item_color(nk_rgba(255, 255, 255, 0));
    toggle->cursor_normal = nk_style_item_image(sprites_ui.ic_check_box);
    toggle->cursor_hover = nk_style_item_image(sprites_ui.ic_check_box_hovered);
    toggle->padding = nk_vec2_s(0.0f, 0.0f);
    toggle->spacing = 0;

    /* option toggle */
    toggle = &style->option;
    toggle->padding = nk_vec2_s(3.0f, 3.0f);
    toggle->spacing = 4 * NK_UI_SCALE;

    /* selectable */
    select = &style->selectable;
    select->padding = nk_vec2_s(2.0f, 2.0f);
    select->image_padding = nk_vec2_s(2.0f, 2.0f);

    /* slider */
    slider = &style->slider;
    slider->cursor_size = nk_vec2_s(16, 16);
    slider->padding = nk_vec2_s(2, 2);
    slider->spacing = nk_vec2_s(2, 2);
    slider->bar_height = 8 * NK_UI_SCALE;

    /* slider buttons */
    button = &style->slider.inc_button;
    button->padding = nk_vec2_s(8.0f, 8.0f);
    button->touch_padding = nk_vec2_s(0.0f, 0.0f);
    button->border = 1.0f * NK_UI_SCALE;

    /* progressbar */
    prog = &style->progress;
    prog->padding = nk_vec2_s(4, 4);

    /* scrollbars */
    scroll = &style->scrollh;

    /* scrollbars buttons */
    button = &style->scrollh.inc_button;
    button->border = 1.0f * NK_UI_SCALE;

    /* edit */
    edit = &style->edit;
    edit->row_padding = 2 * NK_UI_SCALE;
    edit->cursor_size = 4 * NK_UI_SCALE;
    edit->border = 1 * NK_UI_SCALE;

    /* property */
    property = &style->property;
    property->padding = nk_vec2_s(4, 4);
    property->border = 1 * NK_UI_SCALE;
    property->rounding = 10 * NK_UI_SCALE;

    /* property buttons */
    button = &style->property.dec_button;

    /* property edit */
    edit = &style->property.edit;
    edit->cursor_size = 8 * NK_UI_SCALE;

    /* combo */
    combo = &style->combo;
    combo->content_padding = nk_vec2_s(4, 4);
    combo->button_padding = nk_vec2_s(0, 6);
    combo->spacing = nk_vec2_s(4, 0);
    combo->border = 1 * NK_UI_SCALE;

    /* combo button */
    button = &style->combo.button;
    button->padding = nk_vec2_s(2.0f, 2.0f);

    /* tab */
    tab = &style->tab;
    tab->padding = nk_vec2_s(4, 4);
    tab->spacing = nk_vec2_s(4, 4);
    tab->indent = 10.0f * NK_UI_SCALE;
    tab->border = 1 * NK_UI_SCALE;

    /* tab button */
    button = &style->tab.tab_minimize_button;
    button->padding = nk_vec2_s(2.0f, 2.0f);

    /* node button */
    button = &style->tab.node_minimize_button;
    button->padding = nk_vec2_s(2.0f, 2.0f);

    /* window header */
    win = &style->window;
    win->header.label_padding = nk_vec2_s(4, 4);
    win->header.padding = nk_vec2_s(4, 4);

    /* window header close button */
    button = &style->window.header.close_button;

    /* window header minimize button */
    button = &style->window.header.minimize_button;

    /* window */
    win->rounding = 0.0f;
    win->spacing = nk_vec2_s(4, 4);
    win->scrollbar_size = nk_vec2_s(2, 2);
    win->min_size = nk_vec2_s(64, 64);

    win->combo_border = 1.0f * NK_UI_SCALE;
    win->contextual_border = 1.0f * NK_UI_SCALE;
    win->menu_border = 1.0f * NK_UI_SCALE;
    win->group_border = 1.0f * NK_UI_SCALE;
    win->tooltip_border = 1.0f * NK_UI_SCALE;
    win->popup_border = 1.0f * NK_UI_SCALE;
    win->border = 2.0f * NK_UI_SCALE;
    win->min_row_height_padding = 8 * NK_UI_SCALE;

    win->padding = nk_vec2_s(4, 4);
    win->group_padding = nk_vec2_s(4, 4);
    win->popup_padding = nk_vec2_s(4, 4);
    win->combo_padding = nk_vec2_s(4, 4);
    win->contextual_padding = nk_vec2_s(4, 4);
    win->menu_padding = nk_vec2_s(4, 4);
    win->tooltip_padding = nk_vec2_s(4, 4);
}

#endif