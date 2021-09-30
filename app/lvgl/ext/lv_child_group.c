#include "lv_child_group.h"

void cb_child_group_add(lv_event_t *event) {
    lv_obj_t *child = lv_event_get_param(event);
    if (!child || !lv_obj_get_group(child)) return;
    lv_group_t *group = lv_event_get_user_data(event);
    lv_group_add_obj(group, child);
}