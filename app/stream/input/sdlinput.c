#include "app.h"

#include "sdlinput.h"
#include "absinput.h"

#include "ui/root.h"

#include "logging.h"

#if TARGET_WEBOS

#include <SDL_webOS.h>

#endif


static void release_keyboard_keys(SDL_Event ev);

static void sdlinput_handle_input_result(SDL_Event ev, int ret);

static bool absinput_virtual_mouse;

bool absinput_started;
bool absinput_no_control;
bool absinput_no_sdl_mouse;

bool absinput_should_accept() {
    return absinput_started && !ui_should_block_input();
}

void absinput_start() {
    absinput_started = true;
}

void stream_input_stop() {
    absinput_started = false;
}

void release_keyboard_keys(SDL_Event ev) {
}

void absinput_set_virtual_mouse(bool enabled) {
    absinput_virtual_mouse = app_configuration->virtual_mouse && enabled;
}

bool absinput_get_virtual_mouse() {
    return absinput_virtual_mouse;
}