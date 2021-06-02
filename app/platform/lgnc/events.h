#pragma once

#include <linux/input.h>
#include <lgnc_system.h>

#include "util/navkey.h"

struct LGNC_MOUSE_EVENT_T
{
    int posX;
    int posY;
    unsigned int key;
    LGNC_KEY_COND_T keyCond;
    struct input_event raw;
};

struct LGNC_NAVKEY_EVENT_T
{
    NAVKEY navkey;
    NAVKEY_STATE state;
    uint32_t timestamp;
};