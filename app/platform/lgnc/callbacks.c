#include <stdio.h>
#include <string.h>

#include "nuklear/config.h"
#include "nuklear.h"
#include "nuklear/ext_widgets.h"
#include "nuklear/ext_functions.h"
#include "nuklear/platform_lgnc_gles2.h"

#include "main.h"
#include "callbacks.h"
#include "events.h"

#include "util/bus.h"
#include "util/user_event.h"

#include "ui/gui_root.h"
#include "stream/session.h"
#include "stream/input/lgnc.h"

LGNC_STATUS_T _MsgEventHandler(LGNC_MSG_TYPE_T msg, unsigned int submsg, char *pData, unsigned short dataSize)

{
    switch (msg)
    {
    case LGNC_MSG_FOCUS_IN:
        printf("LGNC_MSG_FOCUS_IN\n");
        return LGNC_OK;
    case LGNC_MSG_FOCUS_OUT:
        printf("LGNC_MSG_FOCUS_OUT\n");
        return LGNC_OK;
    case LGNC_MSG_TERMINATE:
        printf("LGNC_MSG_TERMINATE\n");
        break;
    case LGNC_MSG_HOST_EVENT:
        printf("LGNC_MSG_HOST_EVENT\n");
        break;
    case LGNC_MSG_PAUSE:
        printf("LGNC_MSG_PAUSE\n");
        return LGNC_OK;
    case LGNC_MSG_RESUME:
        printf("LGNC_MSG_RESUME\n");
        return LGNC_OK;
    }
    return LGNC_OK;
}

unsigned int _KeyEventCallback(unsigned int key, LGNC_KEY_COND_T keyCond, LGNC_ADDITIONAL_INPUT_INFO_T *keyInput)
{
    printf("KeyEvent key=%d, cond=%d\n", key, keyCond);
    return 1;
}

unsigned int _MouseEventCallback(int posX, int posY, unsigned int key, LGNC_KEY_COND_T keyCond, LGNC_ADDITIONAL_INPUT_INFO_T *keyInput)
{
    // printf("MouseEvent x=%d, y=%d key=%d, cond=%d\n", posX, posY, key, keyCond);
    if (key == 412 /* remote control back */ && keyCond == LGNC_KEY_RELEASE)
    {
        bus_pushevent(USER_QUIT, NULL, NULL);
        return 1;
    }
    if (streaming_status == STREAMING_STREAMING && !gui_should_block_input())
    {
        absinput_dispatch_mouse_event(posX, posY, key, keyCond);
    }
    struct LGNC_MOUSE_EVENT_T *evt = malloc(sizeof(struct LGNC_MOUSE_EVENT_T));
    evt->posX = posX;
    evt->posY = posY;
    evt->key = key;
    evt->keyCond = keyCond;
    bus_pushevent(USER_INPUT_MOUSE, evt, NULL);
    return 0;
}

void _JoystickEventCallback(LGNC_ADDITIONAL_INPUT_INFO_T *e)
{
    struct input_event event = e->event;
    printf("JoystickEvent tv_sec: %lu, type: %04x, code: %04x, value: %08x\n",
           event.time.tv_sec, event.type, event.code, event.value);
}

void _GamepadEventCallback(LGNC_ADDITIONAL_INPUT_INFO_T *e)
{
    struct input_event event = e->event;
    printf("GamepadEvent tv_sec: %lu, type: %04x, code: %04x, value: %08x\n",
           event.time.tv_sec, event.type, event.code, event.value);
}

void _GamepadHotPlugCallback(LGNC_GAMEPAD_INFO *gamepad, int count)
{
    printf("GamepadHotPlug id: %d, type: %08x, count: %d\n",
           gamepad->id, gamepad->type, count);
}