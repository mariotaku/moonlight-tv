//
// Created by Mariotaku on 2021/09/05.
//

#include <SDL_image.h>
#include <res.h>
#include <gpu/sdl/lv_gpu_sdl_utils.h>
#include "appitem.view.h"

static void appitem_holder_free_cb(lv_event_t *event);

static void appitem_draw_decor(lv_event_t *e);

lv_obj_t *appitem_view(apps_controller_t *controller, lv_obj_t *parent) {
    appitem_styles_t *styles = &controller->appitem_style;
    lv_obj_t *item = lv_img_create(parent);
    lv_obj_add_flag(item, LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(item, &styles->cover, 0);

    lv_obj_set_style_outline_opa(item, LV_OPA_COVER, LV_STATE_FOCUS_KEY);

    lv_obj_set_style_transform_zoom(item, 256 * 103 / 100, LV_STATE_PRESSED);
    lv_obj_set_style_transform_zoom(item, 256 * 105 / 100, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_transition(item, &styles->tr_pressed, LV_STATE_PRESSED | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_transition(item, &styles->tr_released, LV_STATE_DEFAULT);
    lv_obj_add_event_cb(item, appitem_draw_decor, LV_EVENT_DRAW_MAIN, styles);

    lv_obj_t *play_indicator = lv_obj_create(item);
    lv_obj_remove_style_all(play_indicator);
    lv_obj_clear_flag(play_indicator, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_style(play_indicator, &styles->btn, 0);
    lv_obj_set_style_bg_img_src(play_indicator, LV_SYMBOL_PLAY, 0);
    lv_obj_center(play_indicator);
    lv_obj_t *title = lv_label_create(item);
    lv_obj_set_size(title, LV_PCT(100), LV_DPX(20));
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
    lv_style_set_bg_opa(&style->btn, LV_OPA_COVER);
    lv_style_set_text_color(&style->btn, lv_color_black());

    static const lv_style_prop_t trans_props[] = {
            LV_STYLE_OUTLINE_OPA, LV_STYLE_TRANSFORM_WIDTH, LV_STYLE_TRANSFORM_HEIGHT, LV_STYLE_TRANSFORM_ZOOM, 0
    };
    lv_style_transition_dsc_init(&style->tr_pressed, trans_props, lv_anim_path_linear, LV_THEME_DEFAULT_TRANSITON_TIME,
                                 0, NULL);
    lv_style_transition_dsc_init(&style->tr_released, trans_props, lv_anim_path_linear, LV_THEME_DEFAULT_TRANSITON_TIME,
                                 70, NULL);

    lv_disp_t *disp = lv_disp_get_default();
    SDL_Renderer *renderer = disp->driver->user_data;
    SDL_Texture *fav_indicator = IMG_LoadTexture_RW(renderer, SDL_RWFromConstMem(res_fav_indicator_data,
                                                                                 (int) res_fav_indicator_size), 1);

    lv_sdl_img_src_t src = {
            .w = LV_DPX(48),
            .h = LV_DPX(48),
            .type = LV_SDL_IMG_TYPE_TEXTURE,
            .cf = LV_IMG_CF_TRUE_COLOR_ALPHA,
            .data.texture = fav_indicator,
    };
    lv_sdl_img_src_stringify(&src, style->fav_indicator_src);
}

void appitem_style_deinit(appitem_styles_t *style) {
    lv_sdl_img_src_t src;
    lv_sdl_img_src_parse(style->fav_indicator_src, &src);
    SDL_DestroyTexture(src.data.texture);
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
    const lv_area_t *clip_area = lv_event_get_param(e);
    lv_area_t coords = {
            zoomed_rect.x, zoomed_rect.y,
            zoomed_rect.x + LV_DPX(48) - 1, zoomed_rect.y + LV_DPX(48) - 1
    };
    lv_draw_img(&coords, clip_area, styles->fav_indicator_src, &dsc);
}