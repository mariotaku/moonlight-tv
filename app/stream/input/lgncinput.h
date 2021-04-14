#pragma once

#include <linux/input.h>
#include <lgnc_system.h>

void absinput_dispatch_mouse_event(int posX, int posY, unsigned int key, LGNC_KEY_COND_T keyCond, struct input_event raw);