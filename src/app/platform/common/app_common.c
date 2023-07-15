#include "app.h"

#include <locale.h>
#include <libintl.h>

#include "util/i18n.h"
#include "app_launch.h"

#define USE_OPENURL ((OS_DARWIN || OS_WINDOWS) && SDL_VERSION_ATLEAST(2, 0, 14))

#if !USE_OPENURL

#include <stdio.h>
#include <stdlib.h>

#endif

void app_open_url(const char *url) {
#if USE_OPENURL
    SDL_OpenURL(url);
#elif OS_LINUX
    char command[8192];
    snprintf(command, sizeof(command), "xdg-open '%s'", url);
    system(command);
#elif OS_DARWIN
    char command[8192];
    snprintf(command, sizeof(command), "open '%s'", url);
    system(command);
#endif
}

void app_init_locale() {
    const char *textdomaindir = getenv("TEXTDOMAINDIR");
    if (textdomaindir) {
        bindtextdomain("moonlight-tv", textdomaindir);
        bind_textdomain_codeset("moonlight-tv", "UTF-8");
    }
    textdomain("moonlight-tv");
    i18n_setlocale("");
}

app_launch_params_t *app_handle_launch(app_t *app, int argc, char *argv[]) {
    // Do nothing
    return NULL;
}