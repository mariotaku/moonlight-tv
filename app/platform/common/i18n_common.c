#include "util/i18n.h"

#include <string.h>

static const i18n_entry_t i18n_locales[] = {
        {"auto", translatable("System Language")},
        {"en-US", "English"},
        {"cs", "Čeština"},
        {"de", "Deutsch"},
        {"es", "Español"},
        {"fr", "Français"},
        {"it", "Italiano"},
#if DEBUG
        {"ja", "日本語"},
#endif
        {"ko", "조선말"},
        {"nl", "Dutch"},
        {"pl", "Polski"},
        {"pt-BR", "Português (Brasil)"},
        {"ro", "Română"},
        {"ru", "Русский"},
        {"zh-CN", "简体中文",
#if TARGET_WINDOWS
                .font =        "Microsoft YaHei"
#endif
        },
        {NULL, NULL},
};

const i18n_entry_t *i18n_entry_at(int index) {
    return &i18n_locales[index];
}

const i18n_entry_t *i18n_entry(const char *locale) {
    if (!locale) return NULL;
    char locale_tmp[16];
    strncpy(locale_tmp, locale, sizeof(locale_tmp) - 1);
    char *current_pos = strchr(locale_tmp, '_');
    while (current_pos) {
        *current_pos = '-';
        current_pos = strchr(current_pos, '_');
    }
    for (int i = 0; i18n_locales[i].locale; i++) {
        const char *item_loc = i18n_locales[i].locale;
        if (strchr(item_loc, '-')) {
            if (strcasecmp(locale_tmp, item_loc) == 0) {
                return &i18n_locales[i];
            }
        } else if (strncasecmp(locale_tmp, item_loc, 2) == 0) {
            return &i18n_locales[i];
        }
    }
    return NULL;
}
