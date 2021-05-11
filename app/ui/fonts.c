#include "res.h"
#include "fonts.h"

#include <stddef.h>

struct nk_font *font_ui_15;
struct nk_font *font_num_40;

void fonts_init(struct nk_font_atlas *atlas, const char *path)
{
    font_ui_15 = nk_font_atlas_add_from_file_s(atlas, path, 15, NULL);

    struct nk_font_config num_cfg = nk_font_config(0);
    static const nk_rune num_ranges[] = {0x0030, 0x0039, 0};
    num_cfg.range = num_ranges;
    font_num_40 = nk_font_atlas_add_from_file_s(atlas, path, 40, &num_cfg);
}