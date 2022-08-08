#include "font.h"
#include "ui/config.h"
#include "i18n.h"
#include "res.h"

#include <fontconfig/fontconfig.h>

app_fontset_t app_iconfonts;

static bool fontset_load_fc(app_fontset_t *set, FcPattern *font);

static bool fontset_load_mem(app_fontset_t *set, const char *name, const void *mem, size_t size);

static void fontset_destroy_fonts(app_fontset_t *fontset);

app_fonts_t *app_font_init(lv_theme_t *theme) {
    app_fontset_t fontset = {.small_size = LV_DPX(14), .normal_size = LV_DPX(16), .large_size = LV_DPX(19)};
    app_iconfonts = fontset;
    if (!fontset_load_mem(&app_iconfonts, "MaterialIcons", res_mat_iconfont_data, res_mat_iconfont_size)) {
        return NULL;
    }
    //does not necessarily have to be a specific name.  You could put anything here and Fontconfig WILL find a font for you
    FcPattern *pattern = FcNameParse((const FcChar8 *) FONT_FAMILY);
    if (!pattern) {
        return NULL;
    }

    FcConfigSubstitute(NULL, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

    FcResult result;

    FcPattern *font = FcFontMatch(NULL, pattern, &result);
    if (font && fontset_load_fc(&fontset, font)) {
        theme->font_normal = fontset.normal;
        theme->font_large = fontset.large;
        theme->font_small = fontset.small;
    }
    if (font) {
        FcPatternDestroy(font);
        font = NULL;
    }
    FcPatternDestroy(pattern);
    pattern = NULL;
#ifdef FONT_FAMILY_FALLBACK
    const i18n_entry_t *loc_entry = i18n_entry(i18n_locale());
    pattern = FcNameParse((const FcChar8 *) ((loc_entry && loc_entry->font) ? loc_entry->font : FONT_FAMILY_FALLBACK));
    if (pattern) {
        FcLangSet *ls = FcLangSetCreate();
        if (loc_entry) {
            FcLangSetAdd(ls, (const FcChar8 *) loc_entry->locale);
            FcPatternAddLangSet(pattern, FC_LANG, ls);
        }

        FcConfigSubstitute(NULL, pattern, FcMatchPattern);
        FcDefaultSubstitute(pattern);

        font = FcFontMatch(NULL, pattern, &result);
        if (font) {
            fontset.fallback = calloc(1, sizeof(app_fontset_t));
            if (fontset_load_fc(fontset.fallback, font)) {
                fontset.normal->fallback = fontset.fallback->normal;
                fontset.large->fallback = fontset.fallback->large;
                fontset.small->fallback = fontset.fallback->small;
            } else {
                free(fontset.fallback);
                fontset.fallback = NULL;
            }
            FcPatternDestroy(font);
            font = NULL;
        }
        FcLangSetDestroy(ls);
        FcPatternDestroy(pattern);
        pattern = NULL;
    }
#endif
    app_fonts_t *fonts = calloc(1, sizeof(app_fonts_t));
    fonts->fonts = fontset;
    fonts->icons = app_iconfonts;
    return fonts;
}

void app_font_deinit(app_fonts_t *fonts) {
    fontset_destroy_fonts(&fonts->fonts);
    fontset_destroy_fonts(&fonts->icons);
    free(fonts);
}

static bool fontset_load_fc(app_fontset_t *set, FcPattern *font) {
    //The pointer stored in 'file' is tied to 'font'; therefore, when 'font' is freed, this pointer is freed automatically.
    //If you want to return the filename of the selected font, pass a buffer and copy the file name into that buffer
    FcChar8 *file = NULL;

    if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) {
        lv_ft_info_t ft_info = {.name=(char *) file, .style = FT_FONT_STYLE_NORMAL, .weight = set->normal_size};
        if (lv_ft_font_init(&ft_info)) {
            set->normal = ft_info.font;
        }
        lv_ft_info_t ft_info_lg = {.name=(char *) file, .style = FT_FONT_STYLE_NORMAL, .weight = set->large_size};
        if (lv_ft_font_init(&ft_info_lg)) {
            set->large = ft_info_lg.font;
        }
        lv_ft_info_t ft_info_sm = {.name=(char *) file, .style = FT_FONT_STYLE_NORMAL, .weight = set->small_size};
        if (lv_ft_font_init(&ft_info_sm)) {
            set->small = ft_info_sm.font;
        }
        return true;
    }
    return false;
}

static bool fontset_load_mem(app_fontset_t *set, const char *name, const void *mem, size_t size) {
    lv_ft_info_t ft_info = {.name = name, .mem=mem, .mem_size = size, .style = FT_FONT_STYLE_NORMAL,
            .weight = set->normal_size};
    if (lv_ft_font_init(&ft_info)) {
        set->normal = ft_info.font;
    }
    lv_ft_info_t ft_info_lg = {.name = name, .mem=mem, .mem_size = size, .style = FT_FONT_STYLE_NORMAL,
            .weight = set->large_size};
    if (lv_ft_font_init(&ft_info_lg)) {
        set->large = ft_info_lg.font;
    }
    lv_ft_info_t ft_info_sm = {.name = name, .mem=mem, .mem_size = size, .style = FT_FONT_STYLE_NORMAL,
            .weight = set->small_size};
    if (lv_ft_font_init(&ft_info_sm)) {
        set->small = ft_info_sm.font;
    }
    if (!set->small || !set->normal || !set->large) {
        if (set->small) {
            lv_ft_font_destroy(set->small);
            set->small = NULL;
        }
        if (set->normal) {
            lv_ft_font_destroy(set->normal);
            set->normal = NULL;
        }
        if (set->large) {
            lv_ft_font_destroy(set->large);
            set->large = NULL;
        }
        return false;
    }
    return true;
}

static void fontset_destroy_fonts(app_fontset_t *fontset) {
    lv_ft_font_destroy(fontset->large);
    lv_ft_font_destroy(fontset->normal);
    lv_ft_font_destroy(fontset->small);
    app_fontset_t *fallback = fontset->fallback;
    if (fallback) {
        lv_ft_font_destroy(fallback->large);
        lv_ft_font_destroy(fallback->normal);
        lv_ft_font_destroy(fallback->small);
        free(fallback);
    }
}