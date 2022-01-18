#include "appitem.view.h"
#include "res.h"

#include "util/font.h"
#include "draw/sdl/lv_draw_sdl_utils.h"

static void appitem_holder_free_cb(lv_event_t *event);

static void appitem_draw_decor(lv_event_t *e);

static void appitem_selected(lv_event_t *e);

static void appitem_deselected(lv_event_t *e);

lv_obj_t *appitem_view(apps_controller_t *controller, lv_obj_t *parent) {
    appitem_styles_t *styles = &controller->appitem_style;
    lv_obj_t *item = lv_img_create(parent);
    lv_obj_add_flag(item, LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(item, &styles->cover, 0);
    lv_obj_set_size(item, controller->col_width, controller->col_height);

    lv_obj_set_style_outline_opa(item, LV_OPA_COVER, LV_STATE_FOCUS_KEY);

    lv_obj_set_style_transform_zoom(item, 256 * 99 / 100, LV_STATE_PRESSED);
    lv_obj_set_style_transform_zoom(item, 256 * 102 / 100, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_transition(item, &styles->tr_pressed, LV_STATE_PRESSED | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_transition(item, &styles->tr_released, LV_STATE_DEFAULT);
    lv_obj_add_event_cb(item, appitem_draw_decor, LV_EVENT_DRAW_MAIN, styles);
    lv_obj_add_event_cb(item, appitem_selected, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(item, appitem_deselected, LV_EVENT_DEFOCUSED, NULL);

    lv_obj_t *play_indicator = lv_obj_create(item);
    lv_obj_clear_flag(play_indicator, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_style_all(play_indicator);
    lv_obj_add_style(play_indicator, &styles->btn, 0);
    lv_obj_set_style_bg_img_src(play_indicator, MAT_SYMBOL_PLAY_ARROW, 0);
    lv_obj_center(play_indicator);
    lv_obj_t *title = lv_label_create(item);
    const lv_font_t *font = lv_theme_get_font_small(item);
    lv_obj_set_style_text_font(title, font, 0);
    lv_obj_set_size(title, LV_PCT(100), lv_obj_get_style_text_font(title, 0)->line_height + LV_DPX(2));
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
    lv_obj_set_style_pad_hor(title, LV_DPX(5), 0);
    lv_obj_align(title, LV_ALIGN_BOTTOM_MID, 0, -lv_dpx(20));

    appitem_viewholder_t *holder = (appitem_viewholder_t *) malloc(sizeof(appitem_viewholder_t));
    memset(holder, 0, sizeof(appitem_viewholder_t));
    holder->controller = controller;
    holder->play_indicator = play_indicator;
    holder->title = title;
    lv_obj_set_user_data(item, holder);
    lv_obj_add_event_cb(item, appitem_holder_free_cb, LV_EVENT_DELETE, holder);
    return item;
}

void appitem_style_init(appitem_styles_t *style) {
    lv_style_init(&style->cover);
    lv_style_set_pad_all(&style->cover, 0);
    lv_style_set_radius(&style->cover, LV_DPX(2));
    lv_style_set_clip_corner(&style->cover, true);
    lv_style_set_shadow_opa(&style->cover, LV_OPA_40);
    lv_style_set_shadow_width(&style->cover, LV_DPX(15));
    lv_style_set_shadow_ofs_y(&style->cover, LV_DPX(3));
    lv_style_set_outline_color(&style->cover, lv_color_lighten(lv_palette_main(LV_PALETTE_BLUE), 30));
    lv_style_set_outline_width(&style->cover, LV_DPX(2));
    lv_style_set_outline_opa(&style->cover, LV_OPA_TRANSP);
    lv_style_set_outline_pad(&style->cover, LV_DPX(3));

    lv_style_init(&style->btn);
    lv_style_set_size(&style->btn, LV_DPX(40));
    lv_style_set_radius(&style->btn, LV_RADIUS_CIRCLE);
    lv_style_set_bg_color(&style->btn, lv_color_white());
    lv_style_set_bg_opa(&style->btn, LV_OPA_COVER);
    lv_style_set_text_color(&style->btn, lv_color_black());
    lv_style_set_border_opa(&style->btn, LV_OPA_TRANSP);
    lv_style_set_text_font(&style->btn, app_iconfonts.large);
    lv_style_set_shadow_width(&style->btn, LV_DPX(4));
    lv_style_set_shadow_color(&style->btn, lv_color_black());
    lv_style_set_shadow_opa(&style->btn, LV_OPA_30);

    static const lv_style_prop_t trans_props[] = {
            LV_STYLE_OUTLINE_OPA, LV_STYLE_TRANSFORM_WIDTH, LV_STYLE_TRANSFORM_HEIGHT, LV_STYLE_TRANSFORM_ZOOM, 0
    };
    lv_style_transition_dsc_init(&style->tr_pressed, trans_props, lv_anim_path_linear, LV_THEME_DEFAULT_TRANSITON_TIME,
                                 0, NULL);
    lv_style_transition_dsc_init(&style->tr_released, trans_props, lv_anim_path_linear, LV_THEME_DEFAULT_TRANSITON_TIME,
                                 70, NULL);
    lv_sdl_img_src_t src = {
            .w = LV_DPX(48),
            .h = LV_DPX(48),
            .type = LV_SDL_IMG_TYPE_CONST_PTR,
            .cf = LV_IMG_CF_TRUE_COLOR_ALPHA,
            .data.constptr = res_fav_indicator_data,
            .data_len = res_fav_indicator_size,
    };
    lv_sdl_img_src_stringify(&src, style->fav_indicator_src);
}

void appitem_style_deinit(appitem_styles_t *style) {
}

static void appitem_holder_free_cb(lv_event_t *event) {
    appitem_viewholder_t *holder = event->user_data;
    free(holder);
}

static void appitem_draw_decor(lv_event_t *e) {
    appitem_styles_t *styles = lv_event_get_user_data(e);
    lv_obj_t *target = lv_event_get_target(e);
    appitem_viewholder_t *holder = lv_obj_get_user_data(target);
    if (!holder->app->fav) return;
    lv_area_t target_coords;
    lv_obj_get_coords(target, &target_coords);
    lv_point_t pivot = {lv_area_get_width(&target_coords) / 2, lv_area_get_height(&target_coords) / 2};
    SDL_Rect zoomed_rect;
    lv_coord_t zoom = lv_obj_get_style_transform_zoom(target, LV_PART_MAIN);
    lv_area_zoom_to_sdl_rect(&target_coords, &zoomed_rect, zoom, &pivot);

    lv_draw_img_dsc_t dsc;
    lv_draw_img_dsc_init(&dsc);
    lv_area_t coords = {
            zoomed_rect.x, zoomed_rect.y,
            zoomed_rect.x + LV_DPX(48) - 1, zoomed_rect.y + LV_DPX(48) - 1
    };
    lv_draw_ctx_t *ctx = lv_event_get_draw_ctx(e);
    lv_draw_img(ctx, &dsc, &coords, styles->fav_indicator_src);
}

static void appitem_selected(lv_event_t *e) {
    lv_obj_t *item = lv_event_get_current_target(e);
    appitem_viewholder_t *holder = lv_obj_get_user_data(item);
    lv_label_set_long_mode(holder->title, LV_LABEL_LONG_SCROLL_CIRCULAR);
}

static void appitem_deselected(lv_event_t *e) {
    lv_obj_t *item = lv_event_get_current_target(e);
    appitem_viewholder_t *holder = lv_obj_get_user_data(item);
    lv_label_set_long_mode(holder->title, LV_LABEL_LONG_DOT);
}