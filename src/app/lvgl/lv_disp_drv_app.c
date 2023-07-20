#include "ui/root.h"
#include "lv_disp_drv_app.h"

#include "draw/sdl/lv_draw_sdl.h"

static void lv_sdl_drv_fb_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *src);

static void lv_sdl_drv_fb_clear(lv_disp_drv_t *disp_drv, uint8_t *buf, uint32_t size);

lv_disp_drv_t *lv_app_disp_drv_create(SDL_Window *window, int dpi) {
    int width = 0, height = 0;
    SDL_GetWindowSize(window, &width, &height);
    LV_ASSERT(width > 0 && height > 0);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    lv_disp_draw_buf_t *draw_buf = lv_mem_alloc(sizeof(lv_disp_draw_buf_t));
    SDL_Texture *texture = lv_draw_sdl_create_screen_texture(renderer, width, height);
    lv_disp_draw_buf_init(draw_buf, texture, NULL, width * height);
    lv_disp_drv_t *driver = lv_mem_alloc(sizeof(lv_disp_drv_t));
    lv_disp_drv_init(driver);

    lv_draw_sdl_drv_param_t *param = lv_mem_alloc(sizeof(lv_draw_sdl_drv_param_t));
    param->renderer = renderer;
    driver->user_data = param;
    driver->draw_buf = draw_buf;
    driver->dpi = dpi;
    driver->flush_cb = lv_sdl_drv_fb_flush;
    driver->clear_cb = lv_sdl_drv_fb_clear;
    driver->hor_res = (lv_coord_t) width;
    driver->ver_res = (lv_coord_t) height;
    SDL_SetRenderTarget(renderer, texture);
    return driver;
}

void lv_app_disp_drv_deinit(lv_disp_drv_t *driver) {
    SDL_DestroyTexture(driver->draw_buf->buf1);
    lv_mem_free(driver->draw_buf);

    lv_draw_sdl_drv_param_t *param = driver->user_data;
    SDL_Renderer *renderer = param->renderer;
    lv_mem_free(param);

    driver->draw_ctx_deinit(driver, driver->draw_ctx);

    /* Textures will be also freed by this call, so free it after draw_ctx */
    SDL_DestroyRenderer(renderer);

    if (driver->draw_ctx != NULL) {
        lv_mem_free(driver->draw_ctx);
        driver->draw_ctx = NULL;
    }
    lv_mem_free(driver);
}

void lv_app_display_resize(lv_disp_t *disp, int width, int height) {
    lv_disp_drv_t *driver = disp->driver;
    lv_draw_sdl_drv_param_t *param = disp->driver->user_data;
    if (driver->draw_buf->buf1) {
        SDL_DestroyTexture(driver->draw_buf->buf1);
    }
    SDL_Texture *texture = lv_draw_sdl_create_screen_texture(param->renderer, width, height);
    lv_disp_draw_buf_init(driver->draw_buf, texture, NULL, width * height);
    driver->hor_res = (lv_coord_t) width;
    driver->ver_res = (lv_coord_t) height;
    SDL_SetRenderTarget(param->renderer, texture);
    lv_disp_drv_update(disp, driver);
}

void lv_app_redraw_now(lv_disp_drv_t *disp_drv) {
    lv_draw_sdl_drv_param_t *param = disp_drv->user_data;
    SDL_Renderer *renderer = param->renderer;
    SDL_Texture *texture = disp_drv->draw_buf->buf1;
    SDL_SetRenderTarget(renderer, NULL);
    if (!ui_render_background()) {
        bool has_renderer = false;
        if (has_renderer) {
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

static void lv_sdl_drv_fb_clear(lv_disp_drv_t *disp_drv, uint8_t *buf, uint32_t size) {
    // No-op
}
