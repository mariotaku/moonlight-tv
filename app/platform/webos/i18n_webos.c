#include "util/i18n.h"

#include <SDL.h>
#include <webosi18n_C.h>

static ResBundleC *bundle = NULL;
static char locale_[8] = "\0";
static char language[4] = "\0";

const char *locstr(const char *msgid) {
    if (!bundle) return msgid;
    return resBundle_getLocString(bundle, msgid);
}

const char *i18n_locale() {
    return locale_;
}

void i18n_setlocale(const char *locale) {
    if (bundle) {
        resBundle_destroy(bundle);
        bundle = NULL;
    }
    SDL_memcpy(locale_, locale, strlen(locale) + 1);
    bundle = resBundle_createWithRootPath(locale, "cstrings.json", SDL_GetBasePath());
    if (SDL_strlen(locale) > 2) {
        SDL_memcpy(language, locale, 2);
        language[2] = '\0';
    }
}

const char *app_get_locale_lang() {
    if (!language[0]) return "C";
    return language;
}