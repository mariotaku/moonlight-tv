#pragma once

#define translatable(str) str

typedef struct i18n_entry_t {
    const char *locale;
    const char *name;
    const char *font;
} i18n_entry_t;

const char *locstr(const char *msgid);

const char *i18n_locale();

const i18n_entry_t *i18n_entry_at(int index);

const i18n_entry_t *i18n_entry(const char *locale);