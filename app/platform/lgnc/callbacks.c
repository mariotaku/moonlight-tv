#include <stdio.h>
#include <string.h>

#include "ui/config.h"

#include "nuklear/config.h"
#include "nuklear.h"
#include "nuklear/ext_functions.h"
#include "nuklear/platform_lgnc_gles2.h"

#include "callbacks.h"
#include "events.h"

#include "util/logging.h"
#include "util/bus.h"
#include "util/user_event.h"

#include "ui/root.h"
#include "stream/session.h"
#include "stream/input/lgncinput.h"

LGNC_STATUS_T _MsgEventHandler(LGNC_MSG_TYPE_T msg, unsigned int submsg, char *pData, unsigned short dataSize)

{
    switch (msg)
    {
    case LGNC_MSG_FOCUS_IN:
        applog_d("LGNC", "LGNC_MSG_FOCUS_IN\n");
        return LGNC_OK;
    case LGNC_MSG_FOCUS_OUT:
        applog_d("LGNC", "LGNC_MSG_FOCUS_OUT\n");
        return LGNC_OK;
    case LGNC_MSG_TERMINATE:
        applog_d("LGNC", "LGNC_MSG_TERMINATE\n");
        break;
    case LGNC_MSG_HOST_EVENT:
        applog_d("LGNC", "LGNC_MSG_HOST_EVENT\n");
        break;
    case LGNC_MSG_PAUSE:
        applog_d("LGNC", "LGNC_MSG_PAUSE\n");
        return LGNC_OK;
    case LGNC_MSG_RESUME:
        applog_d("LGNC", "LGNC_MSG_RESUME\n");
        return LGNC_OK;
    }
    return LGNC_OK;
}

unsigned int _KeyEventCallback(unsigned int key, LGNC_KEY_COND_T keyCond, LGNC_ADDITIONAL_INPUT_INFO_T *keyInput)
{
    applog_d("LGNC", "KeyEvent key=%d, cond=%d\n", key, keyCond);
    return 1;
}

unsigned int _MouseEventCallback(int posX, int posY, unsigned int key, LGNC_KEY_COND_T keyCond, LGNC_ADDITIONAL_INPUT_INFO_T *keyInput)
{
    if (key == LGNC_REMOTE_BACK)
    {
        struct LGNC_NAVKEY_EVENT_T *evt = malloc(sizeof(struct LGNC_NAVKEY_EVENT_T));
        evt->navkey = NAVKEY_CANCEL;
        if (keyCond == LGNC_KEY_PRESS)
            evt->state = NAVKEY_STATE_DOWN;
        else if (keyCond == LGNC_KEY_RELEASE)
            evt->state = NAVKEY_STATE_UP;
        evt->timestamp = keyInput->event.time.tv_sec * 1000 + keyInput->event.time.tv_usec / 1000;
        bus_pushevent(USER_INPUT_NAVKEY, evt, NULL);
        return 1;
    }
    else if (key && key != LGNC_REMOTE_PRESS)
    {
        applog_v("LGNC", "key = %d", key);
        return 1;
    }
    // if (keyCond != LGNC_KEY_COND_LAST)
    struct input_event raw_event = keyInput->event;
    if (streaming_status == STREAMING_STREAMING && !ui_should_block_input())
    {
        absinput_dispatch_mouse_event(posX, posY, key, keyCond, raw_event);
    }
    struct LGNC_MOUSE_EVENT_T *evt = malloc(sizeof(struct LGNC_MOUSE_EVENT_T));
    evt->posX = posX;
    evt->posY = posY;
    evt->key = key;
    evt->keyCond = keyCond;
    evt->raw = raw_event;
    bus_pushevent(USER_INPUT_MOUSE, evt, NULL);
    return 0;
}

void _JoystickEventCallback(LGNC_ADDITIONAL_INPUT_INFO_T *e)
{
    struct input_event event = e->event;
    applog_d("LGNC", "JoystickEvent tv_sec: %lu, type: %04x, code: %04x, value: %08x\n",
             event.time.tv_sec, event.type, event.code, event.value);
}

void _GamepadEventCallback(LGNC_ADDITIONAL_INPUT_INFO_T *e)
{
    struct input_event event = e->event;
    applog_d("LGNC", "GamepadEvent tv_sec: %lu, type: %04x, code: %04x, value: %08x\n",
             event.time.tv_sec, event.type, event.code, event.value);
}

void _GamepadHotPlugCallback(LGNC_GAMEPAD_INFO *gamepad, int count)
{
    applog_d("LGNC", "GamepadHotPlug id: %d, type: %08x, count: %d\n",
             gamepad->id, gamepad->type, count);
}