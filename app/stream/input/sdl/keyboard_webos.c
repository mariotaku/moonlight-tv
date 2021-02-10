#include "stream/input/sdlinput.h"
#include "stream/input/absinput.h"
#include "stream/session.h"

#include <Limelight.h>
#include <SDL.h>

#if OS_WEBOS
#include "platform/sdl/webos_keys.h"
#endif

#include "util/bus.h"
#include "util/user_event.h"

#define VK_0 0x30
#define VK_A 0x41

// These are real Windows VK_* codes
#ifndef VK_F1
#define VK_F1 0x70
#define VK_F13 0x7C
#define VK_NUMPAD0 0x60
#endif

#if OS_WEBOS
static bool sdlinput_handle_key_event_webos(SDL_KeyboardEvent *event, short *keyCode, char *modifiers);
#endif

enum KeyCombo
{
    KeyComboQuit,
    KeyComboUngrabInput,
    KeyComboToggleFullScreen,
    KeyComboToggleStatsOverlay,
    KeyComboToggleMouseMode,
    KeyComboToggleCursorHide,
    KeyComboToggleMinimize,
    KeyComboMax
};

struct SpecialKeyCombo
{
    enum KeyCombo keyCombo;
    SDL_Keycode keyCode;
    SDL_Scancode scanCode;
    bool enabled;
};

static struct SpecialKeyCombo m_SpecialKeyCombos[KeyComboMax] = {
    {KeyComboQuit, SDLK_q, SDL_SCANCODE_Q, true},
    {KeyComboUngrabInput, SDLK_z, SDL_SCANCODE_Z, true},
    {KeyComboToggleFullScreen, SDLK_x, SDL_SCANCODE_X, true},
    {KeyComboToggleStatsOverlay, SDLK_s, SDL_SCANCODE_S, true},
    {KeyComboToggleMouseMode, SDLK_m, SDL_SCANCODE_M, true},
    {KeyComboToggleCursorHide, SDLK_c, SDL_SCANCODE_C, true},
    {KeyComboToggleMinimize, SDLK_d, SDL_SCANCODE_D, true},
};

enum KeyCombo _pending_key_combo = KeyComboMax;

struct KeysDown
{
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
#include "util/linked_list.h"

#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX
#undef LINKEDLIST_DOUBLE

static int keys_find_by_code(struct KeysDown *p, const void *fv)
{
    return p->keyCode == *((short *)fv);
}

static bool isSystemKeyCaptureActive()
{
    return false;
}

void performPendingSpecialKeyCombo()
{
    // The caller must ensure all keys are up
    SDL_assert(keys_len(_pressed_keys) == 0);

    switch (_pending_key_combo)
    {
    case KeyComboQuit:
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "Detected quit key combo");
        streaming_interrupt(false);
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
        bus_pushevent(USER_ST_QUITAPP_CONFIRM, NULL, NULL);
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

void sdlinput_handle_key_event(SDL_KeyboardEvent *event)
{
    short keyCode;
    char modifiers;

    // Check for our special key combos
    if ((event->state == SDL_PRESSED) &&
        (event->keysym.mod & KMOD_CTRL) &&
        (event->keysym.mod & KMOD_ALT) &&
        (event->keysym.mod & KMOD_SHIFT))
    {
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

        if (_pending_key_combo == KeyComboMax)
        {
            for (int i = 0; i < KeyComboMax; i++)
            {
                if (m_SpecialKeyCombos[i].enabled && event->keysym.sym == m_SpecialKeyCombos[i].keyCode)
                {
                    _pending_key_combo = m_SpecialKeyCombos[i].keyCombo;
                    break;
                }
            }
        }

        if (_pending_key_combo == KeyComboMax)
        {
            for (int i = 0; i < KeyComboMax; i++)
            {
                if (m_SpecialKeyCombos[i].enabled && event->keysym.scancode == m_SpecialKeyCombos[i].scanCode)
                {
                    _pending_key_combo = m_SpecialKeyCombos[i].keyCombo;
                    break;
                }
            }
        }
    }

    if (event->state == SDL_PRESSED && _pending_key_combo != KeyComboMax)
    {
        // Ignore further key presses until the special combo is raised
        return;
    }

    if (event->repeat)
    {
        // Ignore repeat key down events
        SDL_assert(event->state == SDL_PRESSED);
        return;
    }

    // Set modifier flags
    modifiers = 0;
    if (event->keysym.mod & KMOD_CTRL)
    {
        modifiers |= MODIFIER_CTRL;
    }
    if (event->keysym.mod & KMOD_ALT)
    {
        modifiers |= MODIFIER_ALT;
    }
    if (event->keysym.mod & KMOD_SHIFT)
    {
        modifiers |= MODIFIER_SHIFT;
    }
    if (event->keysym.mod & KMOD_GUI)
    {
        if (isSystemKeyCaptureActive())
        {
            modifiers |= MODIFIER_META;
        }
    }

    // Set keycode. We explicitly use scancode here because GFE will try to correct
    // for AZERTY layouts on the host but it depends on receiving VK_ values matching
    // a QWERTY layout to work.
    if (event->keysym.scancode >= SDL_SCANCODE_1 && event->keysym.scancode <= SDL_SCANCODE_9)
    {
        // SDL defines SDL_SCANCODE_0 > SDL_SCANCODE_9, so we need to handle that manually
        keyCode = (event->keysym.scancode - SDL_SCANCODE_1) + VK_0 + 1;
    }
    else if (event->keysym.scancode >= SDL_SCANCODE_A && event->keysym.scancode <= SDL_SCANCODE_Z)
    {
        keyCode = (event->keysym.scancode - SDL_SCANCODE_A) + VK_A;
    }
    else if (event->keysym.scancode >= SDL_SCANCODE_F1 && event->keysym.scancode <= SDL_SCANCODE_F12)
    {
        keyCode = (event->keysym.scancode - SDL_SCANCODE_F1) + VK_F1;
    }
    else if (event->keysym.scancode >= SDL_SCANCODE_F13 && event->keysym.scancode <= SDL_SCANCODE_F24)
    {
        keyCode = (event->keysym.scancode - SDL_SCANCODE_F13) + VK_F13;
    }
    else if (event->keysym.scancode >= SDL_SCANCODE_KP_1 && event->keysym.scancode <= SDL_SCANCODE_KP_9)
    {
        // SDL defines SDL_SCANCODE_KP_0 > SDL_SCANCODE_KP_9, so we need to handle that manually
        keyCode = (event->keysym.scancode - SDL_SCANCODE_KP_1) + VK_NUMPAD0 + 1;
    }
    else
    {
        switch (event->keysym.sym)
        {
        case SDL_WEBOS_SCANCODE_LEFT:
            keyCode = 0x25;
            break;
        case SDL_WEBOS_SCANCODE_UP:
            keyCode = 0x26;
            break;
        case SDL_WEBOS_SCANCODE_RIGHT:
            keyCode = 0x27;
            break;
        case SDL_WEBOS_SCANCODE_DOWN:
            keyCode = 0x28;
            break;
        case SDL_WEBOS_SCANCODE_ENTER:
            keyCode = 0x0D;
            break;
        default:
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "Unhandled button event: %d",
                        event->keysym.sym);
            return;
        }
    }

    // Track the key state so we always know which keys are down
    if (event->state == SDL_PRESSED)
    {
        struct KeysDown *node = keys_new();
        node->keyCode = keyCode;
        _pressed_keys = keys_append(_pressed_keys, node);
    }
    else
    {
        struct KeysDown *node = keys_find_by(_pressed_keys, &keyCode, &keys_find_by_code);
        if (node)
        {
            _pressed_keys = keys_remove(_pressed_keys, node);
            free(node);
        }
    }

    if (!absinput_no_control)
    {
        LiSendKeyboardEvent(0x8000 | keyCode,
                            event->state == SDL_PRESSED ? KEY_ACTION_DOWN : KEY_ACTION_UP,
                            modifiers);
    }

    if (_pending_key_combo != KeyComboMax && keys_len(_pressed_keys) == 0)
    {
        int keys;
        const Uint8 *keyState = SDL_GetKeyboardState(&keys);

        // Make sure all client keys are up before we process the special key combo
        for (int i = 0; i < keys; i++)
        {
            if (keyState[i] == SDL_PRESSED)
            {
                return;
            }
        }

        // If we made it this far, no keys are pressed
        performPendingSpecialKeyCombo();
    }
}

#if OS_WEBOS
bool sdlinput_handle_key_event_webos(SDL_KeyboardEvent *event, short *keyCode, char *modifiers)
{
    // TODO Keyboard event on webOS is incorrect
    // https://github.com/mariotaku/moonlight-sdl/issues/4
    switch (event->keysym.sym)
    {
    case SDL_WEBOS_SCANCODE_BACK:
        *keyCode = 0;
        _pending_key_combo = KeyComboToggleStatsOverlay;
        printf("SDL_WEBOS_SCANCODE_BACK pressed\n");
        break;
    case SDL_WEBOS_SCANCODE_YELLOW:
        if (!absinput_no_control)
        {
            LiSendMouseButtonEvent(event->type == SDL_KEYDOWN ? BUTTON_ACTION_PRESS : BUTTON_ACTION_RELEASE,
                                   BUTTON_RIGHT);
        }
        return true;
    default:
        return false;
    }
}
#endif