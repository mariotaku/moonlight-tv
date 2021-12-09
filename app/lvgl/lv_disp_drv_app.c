//
// Created by Mariotaku on 2021/09/10.
//

#include <ui/root.h>
#include <stream/platform.h>
#include <draw/sdl/lv_draw_sdl_utils.h>
#include "draw/sdl/lv_draw_sdl.h"
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
    lv_disp_draw_buf_init(draw_buf, NULL, NULL, width * height);
    lv_disp_drv_t *driver = malloc(sizeof(lv_disp_drv_t));
    lv_disp_drv_init(driver);
    lv_draw_sdl_context_t *context = lv_mem_alloc(sizeof(lv_draw_sdl_context_t));
    lv_draw_sdl_context_init(context);
    context->renderer = renderer;
    context->texture = texture;
    driver->user_data = context;
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

    lv_draw_backend_t *backend = lv_mem_alloc(sizeof(lv_draw_backend_t));
    lv_draw_sdl_backend_init(backend);
    lv_draw_backend_add(backend);
    return disp;
}

void lv_app_display_deinit(lv_disp_t *disp) {
    lv_draw_sdl_context_t *context = disp->driver->user_data;
    lv_draw_sdl_context_deinit(context);
    SDL_DestroyTexture(context->texture);
    SDL_DestroyRenderer(context->renderer);
    lv_mem_free(context);
    lv_mem_free(disp->driver->draw_buf);
    lv_mem_free(disp->driver);
}

void lv_app_display_resize(lv_disp_t *disp, int width, int height) {
    lv_disp_drv_t *driver = disp->driver;
    lv_draw_sdl_context_t *context = disp->driver->user_data;
    if (context->texture) {
        SDL_DestroyTexture(context->texture);
    }
    SDL_Texture *texture = SDL_CreateTexture(context->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET,
                                             width, height);
    context->texture = texture;
    lv_disp_draw_buf_init(driver->draw_buf, NULL, NULL, width * height);
    driver->hor_res = (lv_coord_t) width;
    driver->ver_res = (lv_coord_t) height;
    SDL_RendererInfo renderer_info;
    SDL_GetRendererInfo(context->renderer, &renderer_info);
    SDL_assert(renderer_info.flags & SDL_RENDERER_TARGETTEXTURE);
    SDL_SetRenderTarget(context->renderer, texture);
    lv_disp_drv_update(disp, driver);
}

void lv_app_redraw_now(lv_disp_drv_t *disp_drv) {
    lv_draw_sdl_context_t *context = disp_drv->user_data;
    SDL_Renderer *renderer = context->renderer;
    SDL_Texture *texture = context->texture;
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
    LV_UNUSED(src);
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
    lv_draw_sdl_context_t *context = lv_draw_sdl_get_context();
    SDL_Renderer *renderer = context->renderer;
    SDL_assert(SDL_GetRenderTarget(renderer) == context->texture);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_RenderFillRect(renderer, &rect);
}