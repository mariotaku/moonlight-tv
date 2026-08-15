#include "app.h"
#include "config.h"

#include "pref_obj.h"

#include "util/i18n.h"

typedef struct input_pane_t {
    lv_fragment_t base;

    lv_obj_t *absmouse_toggle;
    lv_obj_t *absmouse_hint;
    lv_obj_t *deadzone_label;

    pref_dropdown_int_entry_t controller_touchpad_mode_entries[3];
    pref_dropdown_int_entry_t controller_touchpad_press_entries[4];

    lv_obj_t *controller_touchpad_sensitivity_label;
    lv_obj_t *controller_touchpad_mouse_controls[6];
} input_pane_t;

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *view);

static void pane_ctor(lv_fragment_t *self, void *args);

static void on_controller_touchpad_mode_changed(lv_event_t *e);

static void on_controller_touchpad_sensitivity_changed(lv_event_t *e);

static void update_controller_touchpad_mouse_options(input_pane_t *pane);

static void update_controller_touchpad_sensitivity_label(input_pane_t *pane);

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
    pane->controller_touchpad_mode_entries[0] = (pref_dropdown_int_entry_t) {
            locstr("Disabled"), CONTROLLER_TOUCHPAD_MODE_DISABLED, false};
    pane->controller_touchpad_mode_entries[1] = (pref_dropdown_int_entry_t) {
            locstr("Mouse"), CONTROLLER_TOUCHPAD_MODE_MOUSE, true};
    pane->controller_touchpad_mode_entries[2] = (pref_dropdown_int_entry_t) {
            locstr("Native"), CONTROLLER_TOUCHPAD_MODE_NATIVE, false};

    pane->controller_touchpad_press_entries[0] = (pref_dropdown_int_entry_t) {
            locstr("Off"), CONTROLLER_TOUCHPAD_PRESS_DISABLED, false};
    pane->controller_touchpad_press_entries[1] = (pref_dropdown_int_entry_t) {
            locstr("Left click"), CONTROLLER_TOUCHPAD_PRESS_LEFT, true};
    pane->controller_touchpad_press_entries[2] = (pref_dropdown_int_entry_t) {
            locstr("Right click"), CONTROLLER_TOUCHPAD_PRESS_RIGHT, false};
    pane->controller_touchpad_press_entries[3] = (pref_dropdown_int_entry_t) {
            locstr("Middle click"), CONTROLLER_TOUCHPAD_PRESS_MIDDLE, false};

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
    lv_obj_t *mode_dropdown = pref_dropdown_int(view,
                                                pane->controller_touchpad_mode_entries,
                                                sizeof(pane->controller_touchpad_mode_entries) /
                                                        sizeof(*pane->controller_touchpad_mode_entries),
                                                &app_configuration->controller_touchpad_mode,
                                                NULL);
    lv_obj_set_width(mode_dropdown, LV_PCT(100));
    lv_obj_add_event_cb(mode_dropdown, on_controller_touchpad_mode_changed, LV_EVENT_VALUE_CHANGED, pane);
    pref_desc_label(view, locstr("Disabled ignores controller touchpad input. Mouse uses it as a relative "
                                 "trackpad. Native forwards controller touch events and the touchpad button "
                                 "to the host."), false);

    size_t mouse_control = 0;
    pane->controller_touchpad_sensitivity_label = pref_title_label(view, NULL);
    lv_obj_t *sensitivity_slider =
            pane->controller_touchpad_mouse_controls[mouse_control++] =
                    pref_slider(view, &app_configuration->controller_touchpad_sensitivity,
                                CONTROLLER_TOUCHPAD_SENSITIVITY_MIN,
                                CONTROLLER_TOUCHPAD_SENSITIVITY_MAX, 5);
    lv_obj_set_width(sensitivity_slider, LV_PCT(100));
    lv_obj_add_event_cb(sensitivity_slider, on_controller_touchpad_sensitivity_changed,
                        LV_EVENT_VALUE_CHANGED, pane);
    pref_desc_label(view, locstr("Adjust relative pointer speed in Mouse mode."), false);

    pane->controller_touchpad_mouse_controls[mouse_control++] =
            pref_checkbox(view, locstr("Tap to click"),
                          &app_configuration->controller_touchpad_tap_to_click, false);
    pref_desc_label(view, locstr("Tap once with one finger to send a left click. "
                                 "Touch and hold briefly to hold left click for dragging."), false);

    pref_title_label(view, locstr("Physical press"));
    lv_obj_t *press_dropdown = pane->controller_touchpad_mouse_controls[mouse_control++] =
            pref_dropdown_int(view,
                              pane->controller_touchpad_press_entries,
                              sizeof(pane->controller_touchpad_press_entries) /
                                      sizeof(*pane->controller_touchpad_press_entries),
                              &app_configuration->controller_touchpad_press,
                              NULL);
    lv_obj_set_width(press_dropdown, LV_PCT(100));
    pref_desc_label(view, locstr("Choose the mouse button sent for a normal physical touchpad press."),
                    false);

    pref_title_label(view, locstr("Secondary click"));
    lv_obj_t *secondary_click_dropdown = pane->controller_touchpad_mouse_controls[mouse_control++] =
            pref_dropdown_int(view,
                              pane->controller_touchpad_press_entries,
                              sizeof(pane->controller_touchpad_press_entries) /
                                      sizeof(*pane->controller_touchpad_press_entries),
                              &app_configuration->controller_touchpad_secondary_click,
                              NULL);
    lv_obj_set_width(secondary_click_dropdown, LV_PCT(100));
    pref_desc_label(view, locstr("Used for a physical press in the lower-right corner or a two-finger tap."),
                    false);

    pane->controller_touchpad_mouse_controls[mouse_control++] =
            pref_checkbox(view, locstr("Two-finger scrolling"),
                          &app_configuration->controller_touchpad_two_finger_scroll, false);
    pref_desc_label(view, locstr("Scroll vertically or horizontally by moving two fingers together."),
                    false);

    pane->controller_touchpad_mouse_controls[mouse_control] =
            pref_checkbox(view, locstr("Invert two-finger scrolling"),
                          &app_configuration->controller_touchpad_invert_two_finger_scroll, false);
    pref_desc_label(view, locstr("Use natural scrolling so content follows your fingers, like on macOS."),
                    false);

    update_controller_touchpad_sensitivity_label(pane);
    update_controller_touchpad_mouse_options(pane);

#if FEATURE_INPUT_EVMOUSE
    hwmouse_state_update(pane);
#endif
    update_deadzone_label(pane);
    return view;
}

static void on_controller_touchpad_mode_changed(lv_event_t *e) {
    input_pane_t *pane = lv_event_get_user_data(e);
    update_controller_touchpad_mouse_options(pane);
}

static void on_controller_touchpad_sensitivity_changed(lv_event_t *e) {
    input_pane_t *pane = lv_event_get_user_data(e);
    update_controller_touchpad_sensitivity_label(pane);
}

static void update_controller_touchpad_mouse_options(input_pane_t *pane) {
    bool mouse_mode = app_configuration->controller_touchpad_mode == CONTROLLER_TOUCHPAD_MODE_MOUSE;
    for (size_t i = 0; i < sizeof(pane->controller_touchpad_mouse_controls) /
                           sizeof(*pane->controller_touchpad_mouse_controls); ++i) {
        if (mouse_mode) {
            lv_obj_clear_state(pane->controller_touchpad_mouse_controls[i], LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(pane->controller_touchpad_mouse_controls[i], LV_STATE_DISABLED);
        }
    }
}

static void update_controller_touchpad_sensitivity_label(input_pane_t *pane) {
    lv_label_set_text_fmt(pane->controller_touchpad_sensitivity_label, "%s - %d%%",
                          locstr("Sensitivity"), app_configuration->controller_touchpad_sensitivity);
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
