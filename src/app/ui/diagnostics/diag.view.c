#include "lvgl.h"
#include "diag.fragment.h"
#include "lvgl/theme/lv_theme_moonlight.h"

void setup_styles(diag_fragment_t *f, lv_obj_t *parent);

lv_obj_t *diag_win_create(lv_fragment_t *self, lv_obj_t *parent) {
    diag_fragment_t *f = (diag_fragment_t *) self;
    setup_styles(f, parent);

    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_remove_style_all(container);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_size(container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_hor(container, LV_DPX(30), 0);
    lv_obj_set_style_pad_ver(container, LV_DPX(20), 0);
    lv_obj_set_style_pad_row(container, LV_DPX(20), 0);


    lv_obj_t *conn_type = lv_obj_create(container);
    lv_obj_remove_style_all(conn_type);
    lv_obj_add_style(conn_type, &f->diag_item_style, 0);

    lv_obj_t *label_conn_type = lv_label_create(conn_type);
    lv_obj_add_style(label_conn_type, &f->diag_title_style, 0);
    lv_label_set_text(label_conn_type, "Connection Type");
    lv_obj_set_flex_grow(label_conn_type, 1);

    lv_obj_t *conn_spinner = lv_spinner_create(conn_type, 1000, 60);
    lv_obj_add_style(conn_spinner, &f->diag_spinner_style, 0);
    lv_obj_add_style(conn_spinner, &f->diag_spinner_style, LV_PART_INDICATOR);

    lv_obj_t *wifi_icon = lv_img_create(conn_type);
    lv_obj_add_style(wifi_icon, &f->large_iconfont_style, 0);
    lv_img_set_src(wifi_icon, MAT_SYMBOL_WIFI);

    lv_obj_t *eth_icon = lv_img_create(conn_type);
    lv_obj_add_style(eth_icon, &f->large_iconfont_style, 0);
    lv_img_set_src(eth_icon, MAT_SYMBOL_SETTINGS_ETHERNET);

    lv_obj_t *conn_hint = lv_label_create(conn_type);
    lv_obj_set_width(conn_hint, LV_PCT(100));
    lv_obj_add_style(conn_hint, &f->hint_item_style, 0);
    lv_label_set_text(conn_hint, "Avoid using both wired and wireless connections at the same time.");

    lv_obj_t *wireless = lv_obj_create(container);
    lv_obj_remove_style_all(wireless);
    lv_obj_add_style(wireless, &f->diag_item_style, 0);

    lv_obj_t *label_wireless = lv_label_create(wireless);
    lv_obj_add_style(label_wireless, &f->diag_title_style, 0);
    lv_label_set_text(label_wireless, "Wi-Fi Connection");
    lv_obj_set_flex_grow(label_wireless, 1);

    lv_obj_t *ch_label = lv_label_create(wireless);
    lv_label_set_text(ch_label, "CH 53 (5GHz)");

    lv_obj_t *wireless_hint = lv_label_create(wireless);
    lv_obj_set_width(wireless_hint, LV_PCT(100));
    lv_obj_add_style(wireless_hint, &f->hint_item_style, 0);
    lv_label_set_text(wireless_hint,
                      "This Wi-Fi channel has DFS enabled. Use another channel without DFS for best performance.");
    return conn_type;
}

void diag_win_deleted(lv_fragment_t *self, lv_obj_t *obj) {
    diag_fragment_t *f = (diag_fragment_t *) self;
    lv_style_reset(&f->diag_item_style);
    lv_style_reset(&f->diag_title_style);
    lv_style_reset(&f->diag_spinner_style);
    lv_style_reset(&f->hint_item_style);
    lv_style_reset(&f->large_iconfont_style);
}

void setup_styles(diag_fragment_t *f, lv_obj_t *parent) {
    lv_style_t *diag_item = &f->diag_item_style;
    lv_style_init(diag_item);
    lv_style_set_layout(diag_item, LV_LAYOUT_FLEX);
    lv_style_set_flex_flow(diag_item, LV_FLEX_FLOW_ROW_WRAP);
    lv_style_set_width(diag_item, LV_PCT(100));
    lv_style_set_height(diag_item, LV_SIZE_CONTENT);
    lv_style_set_pad_column(diag_item, LV_DPX(10));
    lv_style_set_pad_row(diag_item, LV_DPX(15));

    lv_style_t *hint_item = &f->hint_item_style;
    lv_style_init(hint_item);
    lv_style_set_text_font(hint_item, lv_theme_get_font_small(parent));

    lv_style_t *diag_title = &f->diag_title_style;
    lv_style_init(diag_title);
    lv_style_set_text_font(diag_title, lv_theme_get_font_large(parent));

    lv_style_t *spinner = &f->diag_spinner_style;
    lv_style_init(spinner);
    lv_style_set_size(spinner, LV_DPX(20));
    lv_style_set_arc_width(spinner, LV_DPX(3));

    lv_style_t *large_iconfont = &f->large_iconfont_style;
    lv_style_init(large_iconfont);
    lv_style_set_text_font(large_iconfont, lv_theme_moonlight_get_iconfont_large(parent));
}
