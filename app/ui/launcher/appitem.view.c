//
// Created by Mariotaku on 2021/09/05.
//

#include "appitem.view.h"
#include "util/memlog.h"

static void appitem_holder_free_cb(lv_event_t *event);

lv_obj_t *appitem_view(lv_obj_t *parent, appitem_styles_t *styles) {
    lv_obj_t *item = lv_img_create(parent);
    lv_obj_add_flag(item, LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(item, &styles->cover, 0);

    lv_obj_set_style_outline_opa(item, LV_OPA_COVER, LV_STATE_FOCUS_KEY);

    lv_obj_set_style_transform_zoom(item, 256 * 103 / 100, LV_STATE_PRESSED);
    lv_obj_set_style_transform_zoom(item, 256 * 105 / 100, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_transition(item, &styles->tr_pressed, LV_STATE_PRESSED | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_transition(item, &styles->tr_released, LV_STATE_DEFAULT);

    lv_obj_t *play_btn = lv_btn_create(item);
    lv_obj_set_style_bg_img_src(play_btn, LV_SYMBOL_PLAY, 0);
    lv_obj_add_style(play_btn, &styles->btn, 0);
    lv_obj_align(play_btn, LV_ALIGN_CENTER, 0, -lv_dpx(25));
    lv_obj_t *close_btn = lv_btn_create(item);
    lv_obj_set_style_bg_img_src(close_btn, LV_SYMBOL_CLOSE, 0);
    lv_obj_add_style(close_btn, &styles->btn, 0);
    lv_obj_align(close_btn, LV_ALIGN_CENTER, 0, lv_dpx(25));
    lv_obj_t *title = lv_label_create(item);
    lv_obj_align(title, LV_ALIGN_BOTTOM_MID, 0, -lv_dpx(20));

    appitem_viewholder_t *holder = (appitem_viewholder_t *) malloc(sizeof(appitem_viewholder_t));
    memset(holder, 0, sizeof(appitem_viewholder_t));
    holder->play_btn = play_btn;
    holder->close_btn = close_btn;
    holder->title = title;
    lv_obj_set_user_data(item, holder);
    lv_obj_add_event_cb(item, appitem_holder_free_cb, LV_EVENT_DELETE, holder);
    return item;
}

void appitem_style_init(appitem_styles_t *style) {
    lv_style_init(&style->cover);
    lv_style_set_pad_all(&style->cover, 0);
    lv_style_set_radius(&style->cover, 0);
    lv_style_set_shadow_opa(&style->cover, LV_OPA_40);
    lv_style_set_shadow_width(&style->cover, lv_dpx(15));
    lv_style_set_shadow_ofs_y(&style->cover, lv_dpx(3));
    lv_style_set_outline_color(&style->cover, lv_color_lighten(lv_palette_main(LV_PALETTE_BLUE), 30));
    lv_style_set_outline_width(&style->cover, lv_dpx(2));
    lv_style_set_outline_opa(&style->cover, LV_OPA_0);
    lv_style_set_outline_pad(&style->cover, lv_dpx(2));

    lv_style_init(&style->btn);
    lv_style_set_size(&style->btn, lv_dpx(40));
    lv_style_set_radius(&style->btn, LV_RADIUS_CIRCLE);
    lv_style_set_bg_color(&style->btn, lv_color_white());
    lv_style_set_text_color(&style->btn, lv_color_black());

    static const lv_style_prop_t trans_props[] = {
            LV_STYLE_OUTLINE_OPA, LV_STYLE_TRANSFORM_WIDTH, LV_STYLE_TRANSFORM_HEIGHT, LV_STYLE_TRANSFORM_ZOOM, 0
    };
    lv_style_transition_dsc_init(&style->tr_pressed, trans_props, lv_anim_path_linear, LV_THEME_DEFAULT_TRANSITON_TIME,
                                 0, NULL);
    lv_style_transition_dsc_init(&style->tr_released, trans_props, lv_anim_path_linear, LV_THEME_DEFAULT_TRANSITON_TIME,
                                 70, NULL);
}

static void appitem_holder_free_cb(lv_event_t *event) {
    appitem_viewholder_t *holder = event->user_data;
    free(holder);
}