#include "util/i18n.h"

#include <SDL.h>
#include <webosi18n_C.h>

static ResBundleC *bundle = NULL;
static WebOSLocaleC *locale_ = NULL;

const char *locstr(const char *msgid) {
    if (!bundle) return msgid;
    return resBundle_getLocString(bundle, msgid);
}

void i18n_setlocale(const char *locale) {
    if (bundle) {
        resBundle_destroy(bundle);
        bundle = NULL;
    }
    if (locale_) {
        webOSLocale_destroy(locale_);
        locale_ = NULL;
    }
    locale_ = webOSLocale_create(locale);
    bundle = resBundle_createWithRootPath(locale, "cstrings.json", SDL_GetBasePath());
}

const char *app_get_locale() {
    if (!bundle) return "C";
    return resBundle_getLocale(bundle);
}