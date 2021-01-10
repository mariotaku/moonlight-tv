#pragma once

#include <linux/input.h>
#include <lgnc_system.h>

struct LGNC_MOUSE_EVENT_T
{
    int posX;
    int posY;
    unsigned int key;
    LGNC_KEY_COND_T keyCond;
    struct input_event raw;
};