#include "font.h"

bool app_fontset_load(app_fontset_t *set, FcPattern *font) {
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