#include "util/i18n.h"

#include <string.h>

static const i18n_entry_t i18n_locales[] = {
        {"auto", translatable("System Language")},
        {"en-US", "English"},
        {"cs",    "Český Jazyk"},
        {"de",    "Deutsch"},
        {"es",    "Español"},
        {"fr",    "Français"},
        {"it",    "Italiano"},
        {"ja",    "日本語"},
        {"ko",    "조선말"},
        {"nl",    "Dutch"},
        {"pt-BR", "Português (Brasil)"},
        {"ro",    "Română"},
        {"ru",    "Русский"},
        {NULL,   NULL},
};

const i18n_entry_t *i18n_entry_at(int index) {
    return &i18n_locales[index];
}

const i18n_entry_t *i18n_entry(const char *locale) {
    if (!locale) return NULL;
    for (int i = 0; i18n_locales[i].locale; i++) {
        if (strcasecmp(locale, i18n_locales[i].locale) == 0) {
            return &i18n_locales[i];
        }
    }
    return NULL;
}