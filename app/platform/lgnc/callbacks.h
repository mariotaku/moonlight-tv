#pragma once
#include <lgnc_system.h>
#include <lgnc_gamepad.h>

LGNC_STATUS_T _MsgEventHandler(LGNC_MSG_TYPE_T msg, unsigned int submsg, char *pData, unsigned short dataSize);

unsigned int _KeyEventCallback(unsigned int key, LGNC_KEY_COND_T keyCond, LGNC_ADDITIONAL_INPUT_INFO_T *keyInput);

unsigned int _MouseEventCallback(int posX, int posY, unsigned int key, LGNC_KEY_COND_T keyCond, LGNC_ADDITIONAL_INPUT_INFO_T *keyInput);

void _JoystickEventCallback(LGNC_ADDITIONAL_INPUT_INFO_T *e);

void _GamepadEventCallback(LGNC_ADDITIONAL_INPUT_INFO_T *e);

void _GamepadHotPlugCallback(LGNC_GAMEPAD_INFO *gamepad, int count);