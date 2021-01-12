#pragma once
#ifndef NK_NUKLEAR_H_
#include "nuklear.h"
#endif

#ifndef NK_UI_SCALE
#warning "NK_UI_SCALE not defined"
#define NK_UI_SCALE 1
#endif

void nk_ext_apply_style(struct nk_context *ctx);

#ifdef NK_IMPLEMENTATION

void nk_ext_apply_style(struct nk_context *ctx)
{

    struct nk_color table[NK_COLOR_COUNT] = {
    #define NK_COLOR(a,b,c,d,e) {b,c,d,e},
        NK_COLOR_MAP(NK_COLOR)
    #undef NK_COLOR
    };
    table[NK_COLOR_TOGGLE] = nk_rgba(38, 38, 38, 255);
    table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(100, 100, 100, 255);
    table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(60, 60, 60, 255);

    nk_style_from_table(ctx, table);

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
    if (!ctx) return;
    style = &ctx->style;

    /* default button */
    button = &style->button;
    button->padding         = nk_vec2_s(2.0f,2.0f);
    
    /* contextual button */
    button = &style->contextual_button;
    button->padding         = nk_vec2_s(2.0f,2.0f);

    /* menu button */
    button = &style->menu_button;
    button->padding         = nk_vec2_s(2.0f,2.0f);
    
    /* checkbox toggle */
    toggle = &style->checkbox;
    toggle->padding         = nk_vec2_s(0.0f, 0.0f);

    /* option toggle */
    toggle = &style->option;
    toggle->padding         = nk_vec2_s(3.0f, 3.0f);

    /* selectable */
    select = &style->selectable;
    select->padding         = nk_vec2_s(2.0f,2.0f);
    select->image_padding   = nk_vec2_s(2.0f,2.0f);

    /* slider */
    slider = &style->slider;
    slider->cursor_size     = nk_vec2_s(16,16);
    slider->padding         = nk_vec2_s(2,2);
    slider->spacing         = nk_vec2_s(2,2);

    /* slider buttons */
    button = &style->slider.inc_button;
    button->padding         = nk_vec2_s(8.0f,8.0f);

    /* progressbar */
    prog = &style->progress;
    prog->padding           = nk_vec2_s(4,4);

    /* scrollbars */
    scroll = &style->scrollh;

    /* scrollbars buttons */
    button = &style->scrollh.inc_button;

    /* edit */
    edit = &style->edit;

    /* property */
    property = &style->property;
    property->padding       = nk_vec2_s(4,4);

    /* property buttons */
    button = &style->property.dec_button;

    /* property edit */
    edit = &style->property.edit;

    /* combo */
    combo = &style->combo;
    combo->content_padding  = nk_vec2_s(4,4);
    combo->button_padding   = nk_vec2_s(0,6);
    combo->spacing          = nk_vec2_s(4,0);

    /* combo button */
    button = &style->combo.button;
    button->padding         = nk_vec2_s(2.0f,2.0f);

    /* tab */
    tab = &style->tab;
    tab->padding            = nk_vec2_s(4,4);
    tab->spacing            = nk_vec2_s(4,4);

    /* tab button */
    button = &style->tab.tab_minimize_button;
    button->padding         = nk_vec2_s(2.0f,2.0f);

    /* node button */
    button = &style->tab.node_minimize_button;
    button->padding         = nk_vec2_s(2.0f,2.0f);

    /* window header */
    win = &style->window;
    win->header.label_padding = nk_vec2_s(4,4);
    win->header.padding = nk_vec2_s(4,4);

    /* window header close button */
    button = &style->window.header.close_button;

    /* window header minimize button */
    button = &style->window.header.minimize_button;
    
    /* window */
    win->rounding = 0.0f;
    win->spacing = nk_vec2_s(4,4);
    win->scrollbar_size = nk_vec2_s(10,10);
    win->min_size = nk_vec2_s(64,64);

    win->combo_border = 1.0f * NK_UI_SCALE;
    win->contextual_border = 1.0f * NK_UI_SCALE;
    win->menu_border = 1.0f * NK_UI_SCALE;
    win->group_border = 1.0f * NK_UI_SCALE;
    win->tooltip_border = 1.0f * NK_UI_SCALE;
    win->popup_border = 1.0f * NK_UI_SCALE;
    win->border = 2.0f * NK_UI_SCALE;
    win->min_row_height_padding = 8 * NK_UI_SCALE;

    win->padding = nk_vec2_s(4,4);
    win->group_padding = nk_vec2_s(4,4);
    win->popup_padding = nk_vec2_s(4,4);
    win->combo_padding = nk_vec2_s(4,4);
    win->contextual_padding = nk_vec2_s(4,4);
    win->menu_padding = nk_vec2_s(4,4);
    win->tooltip_padding = nk_vec2_s(4,4);

}

#endif