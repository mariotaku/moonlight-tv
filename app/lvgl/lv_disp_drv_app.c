//
// Created by Mariotaku on 2021/09/10.
//

#include <ui/root.h>
#include <util/logging.h>
#include <stream/platform.h>
#include "lv_disp_drv_app.h"

static void lv_sdl_drv_fb_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *src);

static void lv_bg_draw(lv_area_t *area);

lv_disp_t *lv_app_display_init(SDL_Window *window) {
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    lv_disp_draw_buf_t *draw_buf = malloc(sizeof(lv_disp_draw_buf_t));
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, width,
                                             height);
    lv_disp_draw_buf_init(draw_buf, texture, NULL, width * height);
    lv_disp_drv_t *driver = malloc(sizeof(lv_disp_drv_t));
    lv_disp_drv_init(driver);
    driver->user_data = renderer;
    driver->draw_buf = draw_buf;
    driver->flush_cb = lv_sdl_drv_fb_flush;
    driver->hor_res = (lv_coord_t) width;
    driver->ver_res = (lv_coord_t) height;
    SDL_RendererInfo renderer_info;
    SDL_GetRendererInfo(renderer, &renderer_info);
    SDL_assert(renderer_info.flags & SDL_RENDERER_TARGETTEXTURE);
    SDL_SetRenderTarget(renderer, texture);
    lv_disp_t *disp = lv_disp_drv_register(driver);
    disp->bg_color = lv_color_make(0, 0, 0);
    disp->bg_opa = 0;
    disp->bg_fn = lv_bg_draw;
    return disp;
}

void lv_app_display_deinit(lv_disp_t *disp) {
    SDL_DestroyTexture((SDL_Texture *) disp->driver->draw_buf->buf1);
    SDL_DestroyRenderer((SDL_Renderer *) disp->driver->user_data);
    free(disp->driver->draw_buf);
    free(disp->driver);
}

void lv_app_display_resize(lv_disp_t *disp, int width, int height) {
    lv_disp_drv_t *driver = disp->driver;
    SDL_Renderer *renderer = driver->user_data;
    if (driver->draw_buf->buf1) {
        SDL_DestroyTexture(driver->draw_buf->buf1);
    }
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET,
                                             width, height);
    lv_disp_draw_buf_init(driver->draw_buf, texture, NULL, width * height);
    driver->hor_res = (lv_coord_t) width;
    driver->ver_res = (lv_coord_t) height;
    SDL_RendererInfo renderer_info;
    SDL_GetRendererInfo(renderer, &renderer_info);
    SDL_assert(renderer_info.flags & SDL_RENDERER_TARGETTEXTURE);
    SDL_SetRenderTarget(renderer, texture);
    lv_disp_drv_update(disp, driver);
}

void lv_app_redraw_now(lv_disp_drv_t *disp_drv) {
    SDL_Renderer *renderer = (SDL_Renderer *) disp_drv->user_data;
    SDL_Texture *texture = disp_drv->draw_buf->buf_act;
    SDL_SetRenderTarget(renderer, NULL);
    if (!ui_render_background()) {
        if (decoder_info.hasRenderer) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
            SDL_RenderFillRect(renderer, NULL);
        } else {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
            SDL_RenderClear(renderer);
        }
    }
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    SDL_SetRenderTarget(renderer, texture);
}

static void lv_sdl_drv_fb_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *src) {
    (void) src;
    if (area->x2 < 0 || area->y2 < 0 ||
        area->x1 > disp_drv->hor_res - 1 || area->y1 > disp_drv->ver_res - 1) {
        lv_disp_flush_ready(disp_drv);
        return;
    }

    if (lv_disp_flush_is_last(disp_drv)) {
        lv_app_redraw_now(disp_drv);
    }
    lv_disp_flush_ready(disp_drv);
}

static void lv_bg_draw(lv_area_t *area) {
    SDL_Rect rect = {.x=area->x1, .y= area->y1, .w = lv_area_get_width(area), .h = lv_area_get_height(area)};
    lv_disp_t *disp = lv_disp_get_default();
    lv_disp_drv_t *driver = disp->driver;
    SDL_Renderer *renderer = driver->user_data;
    SDL_assert(SDL_GetRenderTarget(renderer) == lv_disp_get_draw_buf(disp)->buf_act);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_RenderSetClipRect(renderer, NULL);
    SDL_RenderFillRect(renderer, &rect);
}