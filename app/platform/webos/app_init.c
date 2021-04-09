#include "app_init.h"
#include "app.h"
#include "stream/settings.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

void app_webos_init(int argc, char *argv[])
{
    SDL_SetHint(SDL_HINT_WEBOS_ACCESS_POLICY_KEYS_BACK, "true");
    SDL_SetHint(SDL_HINT_WEBOS_ACCESS_POLICY_KEYS_EXIT, "true");
    SDL_SetHint(SDL_HINT_WEBOS_CURSOR_SLEEP_TIME, "5000");
    // SDL_SetHint(SDL_HINT_WEBOS_REGISTER_APP, "1");
}

void app_webos_destroy()
{
}

void app_webos_window_setup(SDL_Window *window)
{
}