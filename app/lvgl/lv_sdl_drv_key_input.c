#include <SDL.h>
#include <stream/input/sdlinput.h>
#include <stdint.h>
#include <platform/sdl/navkey_sdl.h>

#include "lvgl.h"
#include "lv_sdl_drv_key_input.h"

typedef struct {
    uint32_t key;
    lv_indev_state_t state;
} indev_key_state_t;

static bool read_event(const SDL_Event *event, indev_key_state_t *state);

static void sdl_input_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    indev_key_state_t *state = drv->user_data;
    SDL_Event e;
    if (SDL_PeepEvents(&e, 1, SDL_GETEVENT, SDL_KEYDOWN, SDL_KEYUP) > 0) {
        if (absinput_dispatch_event(&e)) {
            state->state = LV_INDEV_STATE_RELEASED;
        } else {
            read_event(&e, state);
        }
        data->continue_reading = true;
    } else if (SDL_PeepEvents(&e, 1, SDL_GETEVENT, SDL_CONTROLLERAXISMOTION, SDL_CONTROLLERBUTTONUP) > 0) {
        if (absinput_dispatch_event(&e)) {
            state->state = LV_INDEV_STATE_RELEASED;
        } else {
            read_event(&e, state);
        }
        data->continue_reading = true;
    } else {
        data->continue_reading = false;
    }
    data->key = state->key;
    data->state = state->state;
}

lv_indev_t *lv_sdl_init_key_input() {
    lv_group_t *group = lv_group_get_default();

    lv_indev_drv_t *indev_drv = malloc(sizeof(lv_indev_drv_t));
    lv_indev_drv_init(indev_drv);
    indev_key_state_t *state = malloc(sizeof(indev_key_state_t));
    indev_drv->user_data = state;
    indev_drv->type = LV_INDEV_TYPE_KEYPAD;
    indev_drv->read_cb = sdl_input_read;

    state->state = LV_INDEV_STATE_RELEASED;
    state->key = 0;

    lv_indev_t *indev = lv_indev_drv_register(indev_drv);
    if (group) {
        lv_indev_set_group(indev, group);
    }
    return indev;
}

void lv_sdl_deinit_key_input(lv_indev_t *indev) {
    free(indev->driver->user_data);
    free(indev->driver);
}

static bool read_event(const SDL_Event *event, indev_key_state_t *state) {
    NAVKEY navkey = navkey_from_sdl(event);
    switch (navkey) {
        case NAVKEY_UP:
            state->key = LV_KEY_UP;
            break;
        case NAVKEY_DOWN:
            state->key = LV_KEY_DOWN;
            break;
        case NAVKEY_LEFT:
            state->key = LV_KEY_LEFT;
            break;
        case NAVKEY_RIGHT:
            state->key = LV_KEY_RIGHT;
            break;
        case NAVKEY_MENU:
            state->key = LV_KEY_NEXT;
            break;
        case NAVKEY_CONFIRM:
            state->key = LV_KEY_ENTER;
            break;
        case NAVKEY_CANCEL:
            state->key = LV_KEY_ESC;
            break;
        case NAVKEY_NEGATIVE:
            state->key = LV_KEY_DEL;
            break;
        case NAVKEY_ALTERNATIVE:
            state->key = LV_KEY_BACKSPACE;
            break;
        default:
            return false;
    }
    bool pressed = event->type == SDL_CONTROLLERBUTTONDOWN || event->type == SDL_KEYDOWN;
    state->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    return true;
}