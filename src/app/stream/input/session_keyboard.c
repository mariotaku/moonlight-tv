#include "app.h"
#include "logging.h"
#include "stream/input/session_input.h"
#include "stream/session.h"

#include <Limelight.h>
#include <SDL.h>

#include "util/bus.h"
#include "util/user_event.h"

#include "vk.h"

enum KeyCombo {
    KeyComboQuit,
    KeyComboUngrabInput,
    KeyComboToggleFullScreen,
    KeyComboToggleStatsOverlay,
    KeyComboToggleMouseMode,
    KeyComboToggleCursorHide,
    KeyComboToggleMinimize,
    KeyComboMax
};

struct SpecialKeyCombo {
    enum KeyCombo keyCombo;
    SDL_Keycode keyCode;
    SDL_Scancode scanCode;
    bool enabled;
};

static struct SpecialKeyCombo m_SpecialKeyCombos[KeyComboMax] = {
        {KeyComboQuit,               SDLK_q, SDL_SCANCODE_Q, true},
        {KeyComboUngrabInput,        SDLK_z, SDL_SCANCODE_Z, true},
        {KeyComboToggleFullScreen,   SDLK_x, SDL_SCANCODE_X, true},
        {KeyComboToggleStatsOverlay, SDLK_s, SDL_SCANCODE_S, true},
        {KeyComboToggleMouseMode,    SDLK_m, SDL_SCANCODE_M, true},
        {KeyComboToggleCursorHide,   SDLK_c, SDL_SCANCODE_C, true},
        {KeyComboToggleMinimize,     SDLK_d, SDL_SCANCODE_D, true},
};

enum KeyCombo _pending_key_combo = KeyComboMax;

struct KeysDown {
    short keyCode;
    struct KeysDown *prev;
    struct KeysDown *next;
};

static struct KeysDown *_pressed_keys;

#define LINKEDLIST_IMPL

// Linked list functions for KeysDown
#define LINKEDLIST_TYPE struct KeysDown
#define LINKEDLIST_PREFIX keys
#define LINKEDLIST_DOUBLE 1

#include "linked_list.h"

#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX
#undef LINKEDLIST_DOUBLE

static int keydown_count = 0;

#if TARGET_WEBOS

bool webos_intercept_remote_keys(stream_input_t *input, const SDL_KeyboardEvent *event, short *keyCode);

#endif

static int keys_code_comparator(struct KeysDown *p, const void *fv) {
    return p->keyCode - *((short *) fv);
}

static bool isSystemKeyCaptureActive() {
    return app_configuration->syskey_capture;
}

void performPendingSpecialKeyCombo(stream_input_t *input) {
    // The caller must ensure all keys are up
    SDL_assert_release(keys_len(_pressed_keys) == 0);

    switch (_pending_key_combo) {
        case KeyComboQuit:
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "Detected quit key combo");
            session_interrupt(input->session, false, STREAMING_INTERRUPT_USER);
            break;

        case KeyComboUngrabInput:
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "Detected mouse capture toggle combo");
            break;

        case KeyComboToggleFullScreen:
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "Detected full-screen toggle combo");
            break;

        case KeyComboToggleStatsOverlay:
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "Detected stats toggle combo");
            bus_pushevent(USER_OPEN_OVERLAY, NULL, NULL);
            break;

        case KeyComboToggleMouseMode:
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "Detected mouse mode toggle combo");
            break;
        case KeyComboToggleCursorHide:
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "Detected show mouse combo");
            break;
        case KeyComboToggleMinimize:
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "Detected minimize combo");
            break;
        default:
            break;
    }

    // Reset pending key combo
    _pending_key_combo = KeyComboMax;
}

void stream_input_handle_key(stream_input_t *input, const SDL_KeyboardEvent *event) {
    short keyCode = 0;
#if TARGET_WEBOS
    if (webos_intercept_remote_keys(input, event, &keyCode)) {
        return;
    }
#endif
    char modifiers;

    // Check for our special key combos
    if ((event->state == SDL_PRESSED) &&
        (event->keysym.mod & KMOD_CTRL) &&
        (event->keysym.mod & KMOD_ALT) &&
        (event->keysym.mod & KMOD_SHIFT)) {
        // First we test the SDLK combos for matches,
        // that way we ensure that latin keyboard users
        // can match to the key they see on their keyboards.
        // If nothing matches that, we'll then go on to
        // checking scancodes so non-latin keyboard users
        // can have working hotkeys (though possibly in
        // odd positions). We must do all SDLK tests before
        // any scancode tests to avoid issues in cases
        // where the SDLK for one shortcut collides with
        // the scancode of another.

        if (_pending_key_combo == KeyComboMax) {
            for (int i = 0; i < KeyComboMax; i++) {
                if (m_SpecialKeyCombos[i].enabled && event->keysym.sym == m_SpecialKeyCombos[i].keyCode) {
                    _pending_key_combo = m_SpecialKeyCombos[i].keyCombo;
                    break;
                }
            }
        }

        if (_pending_key_combo == KeyComboMax) {
            for (int i = 0; i < KeyComboMax; i++) {
                if (m_SpecialKeyCombos[i].enabled && event->keysym.scancode == m_SpecialKeyCombos[i].scanCode) {
                    _pending_key_combo = m_SpecialKeyCombos[i].keyCombo;
                    break;
                }
            }
        }
    }

    if (event->state == SDL_PRESSED && _pending_key_combo != KeyComboMax) {
        // Ignore further key presses until the special combo is raised
        return;
    }

    if (event->repeat) {
        // Ignore repeat key down events
        SDL_assert_release(event->state == SDL_PRESSED);
        return;
    }

    // Set modifier flags
    modifiers = 0;
    if (event->keysym.mod & KMOD_CTRL) {
        modifiers |= MODIFIER_CTRL;
    }
    if (event->keysym.mod & KMOD_ALT) {
        modifiers |= MODIFIER_ALT;
    }
    if (event->keysym.mod & KMOD_SHIFT) {
        modifiers |= MODIFIER_SHIFT;
    }
    if (event->keysym.mod & KMOD_GUI) {
        if (isSystemKeyCaptureActive()) {
            modifiers |= MODIFIER_META;
        }
    }

    // Set keycode. We explicitly use scancode here because GFE will try to correct
    // for AZERTY layouts on the host, but it depends on receiving VK_ values matching
    // a QWERTY layout to work.
    if (event->keysym.scancode >= SDL_SCANCODE_1 && event->keysym.scancode <= SDL_SCANCODE_9) {
        // SDL defines SDL_SCANCODE_0 > SDL_SCANCODE_9, so we need to handle that manually
        keyCode = (event->keysym.scancode - SDL_SCANCODE_1) + VK_0 + 1;
    } else if (event->keysym.scancode >= SDL_SCANCODE_A && event->keysym.scancode <= SDL_SCANCODE_Z) {
        keyCode = (event->keysym.scancode - SDL_SCANCODE_A) + VK_A;
    } else if (event->keysym.scancode >= SDL_SCANCODE_F1 && event->keysym.scancode <= SDL_SCANCODE_F12) {
        keyCode = (event->keysym.scancode - SDL_SCANCODE_F1) + VK_F1;
    } else if (event->keysym.scancode >= SDL_SCANCODE_F13 && event->keysym.scancode <= SDL_SCANCODE_F24) {
        keyCode = (event->keysym.scancode - SDL_SCANCODE_F13) + VK_F13;
    } else if (event->keysym.scancode >= SDL_SCANCODE_KP_1 && event->keysym.scancode <= SDL_SCANCODE_KP_9) {
        // SDL defines SDL_SCANCODE_KP_0 > SDL_SCANCODE_KP_9, so we need to handle that manually
        keyCode = (event->keysym.scancode - SDL_SCANCODE_KP_1) + VK_NUMPAD0 + 1;
    } else {
        switch (event->keysym.scancode) {
            case SDL_SCANCODE_BACKSPACE:
                keyCode = VK_BACK;
                break;
            case SDL_SCANCODE_TAB:
                keyCode = VK_TAB;
                break;
            case SDL_SCANCODE_CLEAR:
                keyCode = VK_CLEAR;
                break;
            case SDL_SCANCODE_KP_ENTER: // FIXME: Is this correct?
            case SDL_SCANCODE_RETURN:
                keyCode = VK_RETURN;
                break;
            case SDL_SCANCODE_PAUSE:
                keyCode = VK_PAUSE;
                break;
            case SDL_SCANCODE_CAPSLOCK:
                keyCode = VK_CAPITAL;
                break;
            case SDL_SCANCODE_ESCAPE:
                keyCode = VK_ESCAPE;
                break;
            case SDL_SCANCODE_SPACE:
                keyCode = VK_SPACE;
                break;
            case SDL_SCANCODE_PAGEUP:
                keyCode = VK_PRIOR;
                break;
            case SDL_SCANCODE_PAGEDOWN:
                keyCode = VK_NEXT;
                break;
            case SDL_SCANCODE_END:
                keyCode = VK_END;
                break;
            case SDL_SCANCODE_HOME:
                keyCode = VK_HOME;
                break;
            case SDL_SCANCODE_LEFT:
                keyCode = VK_LEFT;
                break;
            case SDL_SCANCODE_UP:
                keyCode = VK_UP;
                break;
            case SDL_SCANCODE_RIGHT:
                keyCode = VK_RIGHT;
                break;
            case SDL_SCANCODE_DOWN:
                keyCode = VK_DOWN;
                break;
            case SDL_SCANCODE_SELECT:
                keyCode = VK_SELECT;
                break;
            case SDL_SCANCODE_EXECUTE:
                keyCode = VK_EXECUTE;
                break;
            case SDL_SCANCODE_PRINTSCREEN:
                keyCode = VK_SNAPSHOT;
                break;
            case SDL_SCANCODE_INSERT:
                keyCode = VK_INSERT;
                break;
            case SDL_SCANCODE_DELETE:
                keyCode = VK_DELETE;
                break;
            case SDL_SCANCODE_HELP:
                keyCode = VK_HELP;
                break;
            case SDL_SCANCODE_KP_0:
                // See comment above about why we only handle SDL_SCANCODE_KP_0 here
                keyCode = VK_NUMPAD0;
                break;
            case SDL_SCANCODE_0:
                // See comment above about why we only handle SDL_SCANCODE_0 here
                keyCode = VK_0;
                break;
            case SDL_SCANCODE_KP_MULTIPLY:
                keyCode = VK_MULTIPLY;
                break;
            case SDL_SCANCODE_KP_PLUS:
                keyCode = VK_ADD;
                break;
            case SDL_SCANCODE_KP_COMMA:
                keyCode = VK_SEPARATOR;
                break;
            case SDL_SCANCODE_KP_MINUS:
                keyCode = VK_SUBTRACT;
                break;
            case SDL_SCANCODE_KP_PERIOD:
                keyCode = VK_DECIMAL;
                break;
            case SDL_SCANCODE_KP_DIVIDE:
                keyCode = VK_DIVIDE;
                break;
            case SDL_SCANCODE_NUMLOCKCLEAR:
                keyCode = VK_NUMLOCK;
                break;
            case SDL_SCANCODE_SCROLLLOCK:
                keyCode = VK_SCROLL;
                break;
            case SDL_SCANCODE_LSHIFT:
                keyCode = VK_LSHIFT;
                break;
            case SDL_SCANCODE_RSHIFT:
                keyCode = VK_RSHIFT;
                break;
            case SDL_SCANCODE_LCTRL:
                keyCode = VK_LCONTROL;
                break;
            case SDL_SCANCODE_RCTRL:
                keyCode = VK_RCONTROL;
                break;
            case SDL_SCANCODE_LALT:
                keyCode = VK_LMENU;
                break;
            case SDL_SCANCODE_RALT:
                keyCode = VK_RMENU;
                break;
            case SDL_SCANCODE_LGUI:
                if (!isSystemKeyCaptureActive()) {
                    return;
                }
                keyCode = VK_LWIN;
                break;
            case SDL_SCANCODE_RGUI:
                if (!isSystemKeyCaptureActive()) {
                    return;
                }
                keyCode = VK_RWIN;
                break;
            case SDL_SCANCODE_AC_BACK:
                keyCode = VK_BROWSER_BACK;
                break;
            case SDL_SCANCODE_AC_FORWARD:
                keyCode = VK_BROWSER_FORWARD;
                break;
            case SDL_SCANCODE_AC_REFRESH:
                keyCode = VK_BROWSER_REFRESH;
                break;
            case SDL_SCANCODE_AC_STOP:
                keyCode = VK_BROWSER_STOP;
                break;
            case SDL_SCANCODE_AC_SEARCH:
                keyCode = VK_BROWSER_SEARCH;
                break;
            case SDL_SCANCODE_AC_BOOKMARKS:
                keyCode = VK_BROWSER_FAVORITES;
                break;
            case SDL_SCANCODE_AC_HOME:
                keyCode = VK_BROWSER_HOME;
                break;
            case SDL_SCANCODE_SEMICOLON:
                keyCode = VK_OEM_1;
                break;
            case SDL_SCANCODE_EQUALS:
                keyCode = 0xBB;
                break;
            case SDL_SCANCODE_COMMA:
                keyCode = VK_OEM_COMMA;
                break;
            case SDL_SCANCODE_MINUS:
                keyCode = VK_OEM_MINUS;
                break;
            case SDL_SCANCODE_PERIOD:
                keyCode = VK_OEM_PERIOD;
                break;
            case SDL_SCANCODE_SLASH:
                keyCode = VK_OEM_2;
                break;
            case SDL_SCANCODE_GRAVE:
                keyCode = VK_OEM_3;
                break;
            case SDL_SCANCODE_LEFTBRACKET:
                keyCode = VK_OEM_4;
                break;
            case SDL_SCANCODE_BACKSLASH:
                keyCode = VK_OEM_5;
                break;
            case SDL_SCANCODE_RIGHTBRACKET:
                keyCode = VK_OEM_6;
                break;
            case SDL_SCANCODE_APOSTROPHE:
                keyCode = VK_OEM_7;
                break;
            case SDL_SCANCODE_NONUSBACKSLASH:
                keyCode = VK_OEM_102;
                break;
            default:
                if (!keyCode) {
                    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                                "Unhandled button event: scancode: %d, keycode: %d",
                                event->keysym.scancode, event->keysym.sym);
                    return;
                }
        }
    }

    // Track the key state, so we always know which keys are down
    if (event->state == SDL_PRESSED) {
        struct KeysDown *node = keys_new();
        node->keyCode = keyCode;
        _pressed_keys = keys_append(_pressed_keys, node);
    } else {
        struct KeysDown *node = keys_find_by(_pressed_keys, &keyCode, &keys_code_comparator);
        if (node) {
            _pressed_keys = keys_remove(_pressed_keys, node);
            free(node);
        }
    }

    if (!input->view_only) {
        if (event->state == SDL_PRESSED) {
            keydown_count++;
        } else if (event->state == SDL_RELEASED) {
            keydown_count--;
        }
        LiSendKeyboardEvent(0x8000 | keyCode,
                            event->state == SDL_PRESSED ? KEY_ACTION_DOWN : KEY_ACTION_UP,
                            modifiers);
    }

    if (_pending_key_combo != KeyComboMax && keys_len(_pressed_keys) == 0) {
        int keys;
        const Uint8 *keyState = SDL_GetKeyboardState(&keys);

        // Make sure all client keys are up before we process the special key combo
        for (int i = 0; i < keys; i++) {
            if (keyState[i] == SDL_PRESSED) {
                return;
            }
        }

        // If we made it this far, no keys are pressed
        performPendingSpecialKeyCombo(input);
    }
}

void stream_input_handle_text(stream_input_t *input, const SDL_TextInputEvent *event) {
    if (keydown_count) {
        commons_log_verbose("Input", "Ignoring duplicated text input %s. Pressed keys: %d", event->text, keydown_count);
        return;
    }
    size_t len = strlen(event->text);
    if (!len) {
        return;
    }
    LiSendUtf8TextEvent(event->text, len);
}