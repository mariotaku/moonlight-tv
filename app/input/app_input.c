#include "app_input.h"
#include "logging.h"
#include "app.h"

#include "lvgl/lv_sdl_drv_input.h"

static SDL_Surface *blank_surface = NULL;
static SDL_Cursor *blank_cursor = NULL;

void app_input_init(app_input_t *input, app_t *app) {
    blank_surface = SDL_CreateRGBSurface(0, 16, 16, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    blank_cursor = SDL_CreateColorCursor(blank_surface, 0, 0);
    if (!blank_cursor) {
        commons_log_warn("Input", "Failed to create blank cursor: %s", SDL_GetError());
    }

}

void app_input_deinit(app_input_t *input) {

    SDL_FreeCursor(blank_cursor);
    SDL_FreeSurface(blank_surface);
}

void app_stop_text_input(app_input_t *input) {
    SDL_StopTextInput();
}

bool app_text_input_active(app_input_t *input) {
    return SDL_IsTextInputActive();
}

void app_set_mouse_grab(bool grab) {
#if HAVE_RELATIVE_MOUSE_HACK
    if (grab) {
        commons_log_debug("Input", "Set cursor to blank bitmap: %p", blank_cursor);
        SDL_SetCursor(blank_cursor);
    } else {
        SDL_SetCursor(SDL_GetDefaultCursor());
    }
#else
    if (app_configuration->hardware_mouse) {
        SDL_ShowCursor(grab ? SDL_FALSE : SDL_TRUE);
    } else {
        SDL_SetRelativeMouseMode(grab && !app_configuration->absmouse ? SDL_TRUE : SDL_FALSE);
        if (!grab) {
            SDL_ShowCursor(SDL_TRUE);
        }
    }
#endif
}

bool app_get_mouse_relative() {
#if HAVE_RELATIVE_MOUSE_HACK
    return !app_configuration->absmouse;
#else
    return SDL_GetRelativeMouseMode() == SDL_TRUE;
#endif
}
