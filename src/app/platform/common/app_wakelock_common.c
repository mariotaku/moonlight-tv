#include "app.h"

void app_set_keep_awake(app_t *app, bool awake) {
    if (awake) {
        SDL_DisableScreenSaver();
    } else {
        SDL_EnableScreenSaver();
    }
}