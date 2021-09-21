#include <SDL.h>
#include <stream/input/sdlinput.h>

#include "lvgl.h"
#include "lv_sdl_drv_pointer_input.h"

static void indev_pointer_read(lv_indev_drv_t *drv, lv_indev_data_t *data);

static void indev_point_def(lv_indev_data_t *data);

lv_indev_t *lv_sdl_init_pointer() {
    lv_indev_drv_t *indev_drv = malloc(sizeof(lv_indev_drv_t));
    lv_indev_drv_init(indev_drv);
    indev_drv->type = LV_INDEV_TYPE_POINTER;
    indev_drv->read_cb = indev_pointer_read;

    return lv_indev_drv_register(indev_drv);
}


void lv_sdl_deinit_pointer(lv_indev_t *dev) {
    free(dev->driver);
}

static void indev_pointer_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    (void) drv;
    SDL_Event e;
    data->continue_reading = SDL_PeepEvents(&e, 1, SDL_GETEVENT, SDL_MOUSEMOTION, SDL_MOUSEBUTTONUP) > 0;
    if (!data->continue_reading) {
        indev_point_def(data);
        return;
    }
    absinput_dispatch_event(&e);
    if (e.type == SDL_MOUSEMOTION) {
        data->state = e.motion.state;
        data->point = (lv_point_t) {.x = e.motion.x, .y = e.motion.y};
    } else {
        data->state = e.button.state;
        data->point = (lv_point_t) {.x = e.button.x, .y = e.button.y};
    }
}

static void indev_point_def(lv_indev_data_t *data) {
    int x, y;
    data->state = SDL_GetMouseState(&x, &y) != 0;
    data->point.x = (lv_coord_t) x;
    data->point.y = (lv_coord_t) y;
}