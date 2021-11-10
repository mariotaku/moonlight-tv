#include "util/i18n.h"

#include <libintl.h>

const char *locstr(const char *msgid) {
    return gettext(msgid);
}