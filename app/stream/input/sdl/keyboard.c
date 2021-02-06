#include "stream/input/sdlinput.h"
#include "stream/input/absinput.h"
#include "stream/session.h"

#include <Limelight.h>
#include <SDL.h>

#include "platform/sdl/webos_keys.h"

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

enum KeyCombo m_PendingKeyCombo = KeyComboMax;

struct KeysDown
{
    short keyCode;
    struct KeysDown *prev;
    struct KeysDown *next;
};

static struct KeysDown *m_KeysDown;

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
    SDL_assert(keys_len(m_KeysDown) == 0);

    switch (m_PendingKeyCombo)
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
    }

    // Reset pending key combo
    m_PendingKeyCombo = KeyComboMax;
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

        if (m_PendingKeyCombo == KeyComboMax)
        {
            for (int i = 0; i < KeyComboMax; i++)
            {
                if (m_SpecialKeyCombos[i].enabled && event->keysym.sym == m_SpecialKeyCombos[i].keyCode)
                {
                    m_PendingKeyCombo = m_SpecialKeyCombos[i].keyCombo;
                    break;
                }
            }
        }

        if (m_PendingKeyCombo == KeyComboMax)
        {
            for (int i = 0; i < KeyComboMax; i++)
            {
                if (m_SpecialKeyCombos[i].enabled && event->keysym.scancode == m_SpecialKeyCombos[i].scanCode)
                {
                    m_PendingKeyCombo = m_SpecialKeyCombos[i].keyCombo;
                    break;
                }
            }
        }
    }

    if (event->state == SDL_PRESSED && m_PendingKeyCombo != KeyComboMax)
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
#if OS_WEBOS
    else if (event->keysym.sym >= SDLK_WEBOS_BACK && event->keysym.sym <= SDLK_WEBOS_BLUE)
    {
        if (sdlinput_handle_key_event_webos(event, &keyCode))
        {
            return;
        }
    }
#endif
    else
    {
        switch (event->keysym.scancode)
        {
        case SDL_SCANCODE_BACKSPACE:
            keyCode = 0x08;
            break;
        case SDL_SCANCODE_TAB:
            keyCode = 0x09;
            break;
        case SDL_SCANCODE_CLEAR:
            keyCode = 0x0C;
            break;
        case SDL_SCANCODE_KP_ENTER: // FIXME: Is this correct?
        case SDL_SCANCODE_RETURN:
            keyCode = 0x0D;
            break;
        case SDL_SCANCODE_PAUSE:
            keyCode = 0x13;
            break;
        case SDL_SCANCODE_CAPSLOCK:
            keyCode = 0x14;
            break;
        case SDL_SCANCODE_ESCAPE:
            keyCode = 0x1B;
            break;
        case SDL_SCANCODE_SPACE:
            keyCode = 0x20;
            break;
        case SDL_SCANCODE_PAGEUP:
            keyCode = 0x21;
            break;
        case SDL_SCANCODE_PAGEDOWN:
            keyCode = 0x22;
            break;
        case SDL_SCANCODE_END:
            keyCode = 0x23;
            break;
        case SDL_SCANCODE_HOME:
            keyCode = 0x24;
            break;
        case SDL_SCANCODE_LEFT:
            keyCode = 0x25;
            break;
        case SDL_SCANCODE_UP:
            keyCode = 0x26;
            break;
        case SDL_SCANCODE_RIGHT:
            keyCode = 0x27;
            break;
        case SDL_SCANCODE_DOWN:
            keyCode = 0x28;
            break;
        case SDL_SCANCODE_SELECT:
            keyCode = 0x29;
            break;
        case SDL_SCANCODE_EXECUTE:
            keyCode = 0x2B;
            break;
        case SDL_SCANCODE_PRINTSCREEN:
            keyCode = 0x2C;
            break;
        case SDL_SCANCODE_INSERT:
            keyCode = 0x2D;
            break;
        case SDL_SCANCODE_DELETE:
            keyCode = 0x2E;
            break;
        case SDL_SCANCODE_HELP:
            keyCode = 0x2F;
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
            keyCode = 0x6A;
            break;
        case SDL_SCANCODE_KP_PLUS:
            keyCode = 0x6B;
            break;
        case SDL_SCANCODE_KP_COMMA:
            keyCode = 0x6C;
            break;
        case SDL_SCANCODE_KP_MINUS:
            keyCode = 0x6D;
            break;
        case SDL_SCANCODE_KP_PERIOD:
            keyCode = 0x6E;
            break;
        case SDL_SCANCODE_KP_DIVIDE:
            keyCode = 0x6F;
            break;
        case SDL_SCANCODE_NUMLOCKCLEAR:
            keyCode = 0x90;
            break;
        case SDL_SCANCODE_SCROLLLOCK:
            keyCode = 0x91;
            break;
        case SDL_SCANCODE_LSHIFT:
            keyCode = 0xA0;
            break;
        case SDL_SCANCODE_RSHIFT:
            keyCode = 0xA1;
            break;
        case SDL_SCANCODE_LCTRL:
            keyCode = 0xA2;
            break;
        case SDL_SCANCODE_RCTRL:
            keyCode = 0xA3;
            break;
        case SDL_SCANCODE_LALT:
            keyCode = 0xA4;
            break;
        case SDL_SCANCODE_RALT:
            keyCode = 0xA5;
            break;
        case SDL_SCANCODE_LGUI:
            if (!isSystemKeyCaptureActive())
            {
                return;
            }
            keyCode = 0x5B;
            break;
        case SDL_SCANCODE_RGUI:
            if (!isSystemKeyCaptureActive())
            {
                return;
            }
            keyCode = 0x5C;
            break;
        case SDL_SCANCODE_AC_BACK:
            keyCode = 0xA6;
            break;
        case SDL_SCANCODE_AC_FORWARD:
            keyCode = 0xA7;
            break;
        case SDL_SCANCODE_AC_REFRESH:
            keyCode = 0xA8;
            break;
        case SDL_SCANCODE_AC_STOP:
            keyCode = 0xA9;
            break;
        case SDL_SCANCODE_AC_SEARCH:
            keyCode = 0xAA;
            break;
        case SDL_SCANCODE_AC_BOOKMARKS:
            keyCode = 0xAB;
            break;
        case SDL_SCANCODE_AC_HOME:
            keyCode = 0xAC;
            break;
        case SDL_SCANCODE_SEMICOLON:
            keyCode = 0xBA;
            break;
        case SDL_SCANCODE_EQUALS:
            keyCode = 0xBB;
            break;
        case SDL_SCANCODE_COMMA:
            keyCode = 0xBC;
            break;
        case SDL_SCANCODE_MINUS:
            keyCode = 0xBD;
            break;
        case SDL_SCANCODE_PERIOD:
            keyCode = 0xBE;
            break;
        case SDL_SCANCODE_SLASH:
            keyCode = 0xBF;
            break;
        case SDL_SCANCODE_GRAVE:
            keyCode = 0xC0;
            break;
        case SDL_SCANCODE_LEFTBRACKET:
            keyCode = 0xDB;
            break;
        case SDL_SCANCODE_BACKSLASH:
            keyCode = 0xDC;
            break;
        case SDL_SCANCODE_RIGHTBRACKET:
            keyCode = 0xDD;
            break;
        case SDL_SCANCODE_APOSTROPHE:
            keyCode = 0xDE;
            break;
        case SDL_SCANCODE_NONUSBACKSLASH:
            keyCode = 0xE2;
            break;
        default:
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "Unhandled button event: %d",
                        event->keysym.scancode);
            return;
        }
    }

    // Track the key state so we always know which keys are down
    if (event->state == SDL_PRESSED)
    {
        struct KeysDown *node = keys_new();
        node->keyCode = keyCode;
        m_KeysDown = keys_append(m_KeysDown, node);
    }
    else
    {
        struct KeysDown *node = keys_find_by(m_KeysDown, &keyCode, &keys_find_by_code);
        if (node)
        {
            m_KeysDown = keys_remove(m_KeysDown, node);
            free(node);
        }
    }

    if (!absinput_no_control)
    {
        LiSendKeyboardEvent(0x8000 | keyCode,
                            event->state == SDL_PRESSED ? KEY_ACTION_DOWN : KEY_ACTION_UP,
                            modifiers);
    }

    if (m_PendingKeyCombo != KeyComboMax && keys_len(m_KeysDown) == 0)
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

bool sdlinput_handle_key_event_webos(SDL_KeyboardEvent *event, short *keyCode)
{
    // TODO Keyboard event on webOS is incorrect
    // https://github.com/mariotaku/moonlight-sdl/issues/4
    switch (event->keysym.sym)
    {
    case SDLK_WEBOS_BACK:
        *keyCode = 0x1B;
    case SDLK_WEBOS_YELLOW:
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