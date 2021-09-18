#include <SDL.h>
#include <stream/input/sdlinput.h>
#include <stdint.h>
#include <platform/sdl/navkey_sdl.h>

#include "lvgl.h"
#include "lv_sdl_drv_key_input.h"

typedef struct {
    uint32_t key;
    lv_indev_state_t state;
    char text[SDL_TEXTINPUTEVENT_TEXT_SIZE];
    uint32_t text_len;
    uint8_t text_remain;
    uint32_t text_next;
} indev_key_state_t;

static bool read_event(const SDL_Event *event, indev_key_state_t *state);

static bool read_keyboard(const SDL_KeyboardEvent *event, indev_key_state_t *state);

static void sdl_input_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    indev_key_state_t *state = drv->user_data;
    SDL_Event e;
    if (state->text_remain > 0) {
        if (state->state == LV_INDEV_STATE_PRESSED) {
            state->state = LV_INDEV_STATE_RELEASED;
            state->text_remain--;
            data->continue_reading = state->text_remain > 0;
        } else {
            state->key = 0;
            memcpy(&state->key, &state->text[state->text_next],
                   _lv_txt_encoded_size(&state->text[state->text_next]));
            _lv_txt_encoded_next(state->text, &state->text_next);
            state->state = LV_INDEV_STATE_PRESSED;
            data->continue_reading = true;
        }
    } else if (SDL_PeepEvents(&e, 1, SDL_GETEVENT, SDL_KEYDOWN, SDL_KEYUP) > 0) {
        if (absinput_dispatch_event(&e)) {
            state->state = LV_INDEV_STATE_RELEASED;
        } else {
            read_keyboard(&e.key, state);
        }
        data->continue_reading = true;
    } else if (SDL_PeepEvents(&e, 1, SDL_GETEVENT, SDL_CONTROLLERAXISMOTION, SDL_CONTROLLERBUTTONUP) > 0) {
        if (absinput_dispatch_event(&e)) {
            state->state = LV_INDEV_STATE_RELEASED;
        } else {
            read_event(&e, state);
        }
        data->continue_reading = true;
    } else if (SDL_PeepEvents(&e, 1, SDL_GETEVENT, SDL_TEXTINPUT, SDL_TEXTINPUT) > 0) {
        if (absinput_dispatch_event(&e)) {
            state->state = LV_INDEV_STATE_RELEASED;
        } else {
            uint8_t size = _lv_txt_get_encoded_length(e.text.text);
            if (size > 0) {
                state->text_len = strlen(e.text.text);
                state->text_remain = size;
                state->text_next = 0;
                SDL_memcpy(state->text, e.text.text, SDL_TEXTINPUTEVENT_TEXT_SIZE);
                state->key = 0;
                state->state = LV_INDEV_STATE_RELEASED;
            }
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
    lv_memset_00(state, sizeof(indev_key_state_t));
    indev_drv->user_data = state;
    indev_drv->type = LV_INDEV_TYPE_KEYPAD;
    indev_drv->read_cb = sdl_input_read;

    state->state = LV_INDEV_STATE_RELEASED;

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
            switch (event->type) {
                case SDL_KEYDOWN:
                case SDL_KEYUP: {

                }
                default: {
                    break;
                }
            }
            return false;
    }

    handled:
    (void) 0;
    bool pressed = event->type == SDL_CONTROLLERBUTTONDOWN || event->type == SDL_KEYDOWN;
    state->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    return true;
}

static bool read_keyboard(const SDL_KeyboardEvent *event, indev_key_state_t *state) {
    switch (event->keysym.sym) {
        case SDLK_UP:
            state->key = LV_KEY_UP;
            break;
        case SDLK_DOWN:
            state->key = LV_KEY_DOWN;
            break;
        case SDLK_RIGHT:
            state->key = LV_KEY_RIGHT;
            break;
        case SDLK_LEFT:
            state->key = LV_KEY_LEFT;
            break;
        case SDLK_ESCAPE:
            state->key = LV_KEY_ESC;
            break;
        case SDLK_DELETE:
            state->key = LV_KEY_DEL;
            break;
        case SDLK_BACKSPACE:
            state->key = LV_KEY_BACKSPACE;
            break;
        case SDLK_KP_ENTER:
        case SDLK_RETURN:
        case SDLK_RETURN2:
            state->key = LV_KEY_ENTER;
            break;
        case SDLK_TAB:
            state->key = (event->keysym.mod & KMOD_SHIFT) ? LV_KEY_PREV : LV_KEY_NEXT;
            break;
        case SDLK_HOME:
            state->key = LV_KEY_HOME;
            break;
        case SDLK_END:
            state->key = LV_KEY_END;
            break;
        default:
            return false;
    }
    bool pressed = event->state == SDL_PRESSED;
    state->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    return true;
}