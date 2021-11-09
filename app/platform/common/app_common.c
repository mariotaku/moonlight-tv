#include "app.h"

#include <libintl.h>
#include <locale.h>

#include "util/logging.h"

void app_open_url(const char *url) {
#if SDL_VERSION_ATLEAST(2, 0, 14)
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
    textdomain("moonlight-tv");
    applog_i("APP", "UI locale: %s (%s)", setlocale(LC_MESSAGES, NULL), gettext("[Localized Language]"));
}