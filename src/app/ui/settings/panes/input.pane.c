#include "app.h"
#include "config.h"

#include "pref_obj.h"

#include "util/i18n.h"

typedef struct input_pane_t {
    lv_fragment_t base;

    lv_obj_t *absmouse_toggle;
    lv_obj_t *absmouse_hint;
    lv_obj_t *deadzone_label;

    pref_dropdown_int_entry_t touchpad_mode_entries[2];
    lv_obj_t *touchpad_speed_label;
} input_pane_t;

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *view);

static void pane_ctor(lv_fragment_t *self, void *args);

static void on_touchpad_speed_changed(lv_event_t *e);

static void update_touchpad_speed_label(input_pane_t *pane);

#if FEATURE_INPUT_EVMOUSE
static void hwmouse_state_update_cb(lv_event_t *e);

static void hwmouse_state_update(input_pane_t *pane);
#endif

static void update_deadzone_label(input_pane_t *pane);

static void on_deadzone_changed(lv_event_t *e);

const lv_fragment_class_t settings_pane_input_cls = {
        .constructor_cb = pane_ctor,
        .create_obj_cb = create_obj,
        .instance_size = sizeof(input_pane_t),
};

static void pane_ctor(lv_fragment_t *self, void *args) {

}

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *container) {
    input_pane_t *pane = (input_pane_t *) self;
    lv_obj_t *view = pref_pane_container(container);
    lv_obj_set_layout(view, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(view, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(view, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    pane->touchpad_mode_entries[0] = (pref_dropdown_int_entry_t) {
            locstr("Send as mouse"), TOUCHPAD_MODE_MOUSE, false};
    pane->touchpad_mode_entries[1] = (pref_dropdown_int_entry_t) {
            locstr("Send as touchpad"), TOUCHPAD_MODE_NATIVE, true};

    pref_checkbox(view, locstr("View-only mode"), &app_configuration->viewonly, false);
    pref_desc_label(view, locstr("Don't send mouse, keyboard or gamepad input to host computer."), false);

    pref_checkbox(view, locstr("Capture system keys"), &app_configuration->syskey_capture, false);
    pref_desc_label(view, locstr("Capture and send system keys (e.g. Meta/Win key) to host computer."), false);

    pref_header(view, locstr("Mouse"));

#if FEATURE_INPUT_EVMOUSE
    lv_obj_t *hwmouse_toggle = pref_checkbox(view, locstr("Use mouse hardware"),
                                             &app_configuration->hardware_mouse, false);
    lv_obj_add_event_cb(hwmouse_toggle, hwmouse_state_update_cb, LV_EVENT_VALUE_CHANGED, pane);
    pref_desc_label(view, locstr("Use plugged mouse device only when streaming. "
                                 "This will have better performance, but absolute mouse mode will not be enabled."),
                    false);
#endif

    pane->absmouse_toggle = pref_checkbox(view, locstr("Absolute mouse mode"),
                                          &app_configuration->absmouse, false);
    pane->absmouse_hint = pref_desc_label(view, locstr("Better for remote desktop. "
                                                       "For some games, mouse will not work properly."), false);

    pref_header(view, locstr("Gamepad"));

    pane->deadzone_label = pref_title_label(view, locstr("Analog stick deadzone"));
    lv_obj_t *deadzone_slider = pref_slider(view, &app_configuration->stick_deadzone, 0, 20, 1);
    lv_obj_set_width(deadzone_slider, LV_PCT(100));
    lv_obj_add_event_cb(deadzone_slider, on_deadzone_changed, LV_EVENT_VALUE_CHANGED, pane);
    pref_desc_label(view, locstr("Note: Some games can enforce a larger deadzone "
                                 "than what Moonlight is configured to use."), false);

    pref_checkbox(view, locstr("Virtual mouse"), &app_configuration->virtual_mouse, false);
    pref_desc_label(view, locstr("Press LB + RS to move mouse cursor with sticks. "
                                 "LT/RT for left/right mouse buttons."), false);

    pref_checkbox(view, locstr("Swap ABXY buttons"), &app_configuration->swap_abxy, false);
    pref_desc_label(view, locstr("Swap A/B and X/Y gamepad buttons. Useful when you prefer Nintendo-like layouts."),
                    false);

    pref_header(view, locstr("Controller touchpad"));

    pref_title_label(view, locstr("Mode"));
    lv_obj_t *mode_dropdown = pref_dropdown_int(view, pane->touchpad_mode_entries,
                                                sizeof(pane->touchpad_mode_entries) /
                                                        sizeof(*pane->touchpad_mode_entries),
                                                &app_configuration->touchpad_mode, NULL);
    lv_obj_set_width(mode_dropdown, LV_PCT(100));
    pref_desc_label(view, locstr("Send as mouse uses the touchpad as a trackpad. "
                                 "Send as touchpad forwards touches to the host, for games "
                                 "with built-in touchpad support."), false);

    pane->touchpad_speed_label = pref_title_label(view, NULL);
    lv_obj_t *sensitivity_slider = pref_slider(view, &app_configuration->touchpad_speed,
                                               TOUCHPAD_SPEED_MIN,
                                               TOUCHPAD_SPEED_MAX, 5);
    lv_obj_set_width(sensitivity_slider, LV_PCT(100));
    lv_obj_add_event_cb(sensitivity_slider, on_touchpad_speed_changed,
                        LV_EVENT_VALUE_CHANGED, pane);
    pref_desc_label(view, locstr("Adjust pointer speed when the touchpad is used as a mouse."), false);

    pref_checkbox(view, locstr("Multi-touch gestures"),
                  &app_configuration->touchpad_multitouch, false);
    pref_desc_label(view, locstr("Use two fingers to scroll and to right click. "
                                 "Turn off if you trigger them by accident."), false);

    pref_checkbox(view, locstr("Natural scrolling"),
                  &app_configuration->touchpad_natural_scroll, false);
    pref_desc_label(view, locstr("Scrolling follows your fingers, like on a phone."), false);

    update_touchpad_speed_label(pane);

#if FEATURE_INPUT_EVMOUSE
    hwmouse_state_update(pane);
#endif
    update_deadzone_label(pane);
    return view;
}

static void on_touchpad_speed_changed(lv_event_t *e) {
    input_pane_t *pane = lv_event_get_user_data(e);
    update_touchpad_speed_label(pane);
}

static void update_touchpad_speed_label(input_pane_t *pane) {
    lv_label_set_text_fmt(pane->touchpad_speed_label, "%s - %d%%",
                          locstr("Pointer speed"), app_configuration->touchpad_speed);
}

#if FEATURE_INPUT_EVMOUSE
static void hwmouse_state_update_cb(lv_event_t *e) {
    hwmouse_state_update((input_pane_t *) lv_event_get_user_data(e));
}

static void hwmouse_state_update(input_pane_t *pane) {
    if (app_configuration->hardware_mouse) {
        lv_obj_add_state(pane->absmouse_toggle, LV_STATE_DISABLED);
        lv_label_set_text(pane->absmouse_hint, locstr("Absolute mouse mode can't be used when "
                                                      "\"Use mouse hardware\" enabled."));
    } else {
        lv_obj_clear_state(pane->absmouse_toggle, LV_STATE_DISABLED);
        lv_label_set_text(pane->absmouse_hint, locstr("Better for remote desktop. "
                                                      "For some games, mouse will not work properly."));
    }
}
#endif

static void update_deadzone_label(input_pane_t *pane) {
    lv_label_set_text_fmt(pane->deadzone_label, "%s - %d", locstr("Analog stick deadzone"),
                          app_configuration->stick_deadzone);
}

static void on_deadzone_changed(lv_event_t *e) {
    input_pane_t *pane = (input_pane_t *) lv_event_get_user_data(e);
    update_deadzone_label(pane);
}
