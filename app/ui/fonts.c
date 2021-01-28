#include "res.h"
#include "fonts.h"

#include <stddef.h>

struct nk_font *font_ui_15;

void fonts_init(struct nk_font_atlas *atlas)
{
    font_ui_15 = nk_font_atlas_add_from_memory_s(atlas, (void *)res_notosans_regular_data, res_notosans_regular_size, 15, NULL);
}