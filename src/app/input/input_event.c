#include "app_input.h"
#include "input_gamepad.h"

#include <SDL_events.h>

#include "logging.h"

void app_input_handle_event(app_input_t *input, const SDL_Event *event) {
    if (event->type == SDL_JOYDEVICEADDED) {
        if (app_input_gamepads(input) >= app_input_max_gamepads(input)) {
            // Ignore controllers more than supported
            commons_log_warn("Input", "Too many controllers, ignoring.");
            return;
        }
        app_input_init_gamepad(input,event->jdevice.which);
    } else if (event->type == SDL_JOYDEVICEREMOVED) {
        app_input_close_gamepad(input,event->jdevice.which);
    } else if (event->type == SDL_CONTROLLERDEVICEADDED) {
        commons_log_debug("Input", "SDL_CONTROLLERDEVICEADDED");
    } else if (event->type == SDL_CONTROLLERDEVICEREMOVED) {
        commons_log_debug("Input", "SDL_CONTROLLERDEVICEREMOVED");
    } else if (event->type == SDL_CONTROLLERDEVICEREMAPPED) {
        commons_log_debug("Input", "SDL_CONTROLLERDEVICEREMAPPED");
    }
}
