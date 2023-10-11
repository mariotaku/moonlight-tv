#include "app_input.h"
#include "logging.h"
#include "app.h"
#include "input_gamepad_mapping.h"

#include "lvgl/lv_sdl_drv_input.h"

void app_input_init(app_input_t *input, app_t *app) {
    if (app->settings.condb_path != NULL) {
        app_input_copy_initial_gamepad_mapping(&app->settings);
#if SDL_VERSION_ATLEAST(2, 0, 10)
        SDL_SetHint(SDL_HINT_GAMECONTROLLERCONFIG_FILE, app->settings.condb_path);
#endif
    }
    SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
    input->max_num_gamepads = 4;
    input->gamepads_count = 0;
    for(int i = 0; i < input->max_num_gamepads; i++) {
        input->gamepads[i].instance_id = -1;
        input->gamepads[i].gs_id = -1;
    }
    input->activeGamepadMask = 0;
    input->blank_cursor_surface = SDL_CreateRGBSurface(0, 16, 16, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    input->blank_cursor_surface->userdata = SDL_CreateColorCursor(input->blank_cursor_surface, 0, 0);
    if (input->blank_cursor_surface->userdata == NULL) {
        commons_log_warn("Input", "Failed to create blank cursor: %s", SDL_GetError());
    }
#if !SDL_VERSION_ATLEAST(2, 0, 10)
    SDL_GameControllerAddMappingsFromFile(app->settings.condb_path);
#endif
    app_input_init_gamepad_mapping(input, app->backend.executor, &app->settings);
}

void app_input_deinit(app_input_t *input) {
    app_input_deinit_gamepad_mapping(input);
    if (input->blank_cursor_surface->userdata != NULL) {
        SDL_FreeCursor(input->blank_cursor_surface->userdata);
    }
    SDL_FreeSurface(input->blank_cursor_surface);
    SDL_QuitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
}

void app_set_mouse_grab(app_input_t *input, bool grab) {
#if HAVE_RELATIVE_MOUSE_HACK
    if (grab && input->blank_cursor_surface->userdata != NULL) {
        commons_log_debug("Input", "Set cursor to blank bitmap: %p", input->blank_cursor_surface->userdata);
        SDL_SetCursor(input->blank_cursor_surface->userdata);
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
