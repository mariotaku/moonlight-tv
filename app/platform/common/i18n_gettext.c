#include "util/i18n.h"

#include <locale.h>
#include <libintl.h>
#include <string.h>

const char *locstr(const char *msgid) {
    return gettext(msgid);
}

const char *app_get_locale() {
    return setlocale(LC_MESSAGES, NULL);
}