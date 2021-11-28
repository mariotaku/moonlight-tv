#include "app.h"
#include "help.dialog.h"

#include "util/i18n.h"

static void menu_key_cb(lv_event_t *e);

static void help_dialog_resize_cb(lv_event_t *e);

static void help_open_url_cb(lv_event_t *e);

static void add_url_button(lv_obj_t *list, const char *title, const char *url);

lv_obj_t *help_dialog_create() {
    lv_obj_t *msgbox = lv_msgbox_create(NULL, locstr("Help"), NULL, NULL, true);
    lv_obj_clear_flag(msgbox, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(lv_obj_get_parent(msgbox), LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(msgbox, help_dialog_resize_cb, LV_EVENT_SIZE_CHANGED, NULL);
    lv_obj_set_size(msgbox, LV_PCT(80), LV_PCT(90));

    lv_obj_t *close = lv_msgbox_get_close_btn(msgbox);
    lv_group_remove_obj(close);

    lv_obj_t *content = lv_msgbox_get_content(msgbox);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_add_flag(content, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_t *list = lv_list_create(content);
    lv_obj_add_event_cb(list, menu_key_cb, LV_EVENT_KEY, msgbox);
    lv_obj_set_size(list, LV_PCT(100), LV_PCT(100));
    lv_obj_add_flag(list, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_style_border_side(list, LV_BORDER_SIDE_NONE, 0);
    lv_obj_set_style_radius(list, 0, 0);
    lv_obj_set_style_pad_all(list, 0, 0);


    lv_list_add_text(list, "Input");
    add_url_button(list, locstr("Gamepad hotkeys"),
                   "https://github.com/mariotaku/moonlight-tv/wiki/Input-Shortcuts#gamepad");
    add_url_button(list, locstr("Remote controller keys"),
                   "https://github.com/mariotaku/moonlight-tv/wiki/Input-Shortcuts#webos-remote-controller");
    lv_list_add_text(list, "More help");
    add_url_button(list, locstr("Open moonlight-tv Wiki"),
                   "https://github.com/mariotaku/moonlight-tv/wiki");
    add_url_button(list, locstr("Open moonlight-stream Wiki"),
                   "https://github.com/moonlight-stream/moonlight-docs/wiki");

    lv_obj_center(msgbox);
    return msgbox;
}

static void menu_key_cb(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);
    if (target->parent != lv_event_get_current_target(e)) return;
    if (lv_event_get_key(e) == LV_KEY_ESC) {
        lv_msgbox_close_async(lv_event_get_user_data(e));
    }
}

static void help_dialog_resize_cb(lv_event_t *e) {
    lv_obj_t *msgbox = lv_event_get_current_target(e);
    lv_obj_t *title = lv_msgbox_get_title(msgbox);
    lv_obj_t *close_btn = lv_msgbox_get_close_btn(msgbox);
    lv_coord_t title_height = LV_MAX(lv_obj_get_height(title), lv_obj_get_height(close_btn));
    lv_obj_t *content = lv_msgbox_get_content(msgbox);
    lv_coord_t height = lv_obj_get_content_height(msgbox);
    lv_obj_set_height(content, height - title_height);
}

static void help_open_url_cb(lv_event_t *e) {
    app_open_url(lv_event_get_user_data(e));
}

static void add_url_button(lv_obj_t *list, const char *title, const char *url) {
    lv_obj_t *item = lv_list_add_btn(list, NULL, locstr(title));
    lv_obj_add_flag(item, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(item, help_open_url_cb, LV_EVENT_CLICKED, (void *) url);
}