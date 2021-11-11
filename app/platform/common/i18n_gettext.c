#include "util/i18n.h"

#include <libintl.h>

const char *locstr(const char *msgid) {
    return gettext(msgid);
}

const char *app_get_locale() {
    return setlocale(LC_MESSAGES, NULL);
}

const char *app_get_locale_lang() {
    return setlocale(LC_MESSAGES, NULL);
}