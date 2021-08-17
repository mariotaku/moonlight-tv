/* MIT License
 * 
 * Copyright (c) [2020] [Ryan Wendland]
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <SDL.h>

#include "lvgl.h"
#include "lv_conf.h"
#include "lv_sdl_drv_key_input.h"

static void sdl_input_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void)drv;
    data->key = 0;
    static SDL_Event e;
    if (SDL_PeepEvents(&e, 1, SDL_GETEVENT, SDL_KEYDOWN, SDL_KEYUP) > 0)
    {
        if (e.type == SDL_KEYDOWN)
            data->state = LV_INDEV_STATE_PR;
        if (e.type == SDL_KEYUP)
            data->state = LV_INDEV_STATE_REL;

        switch (e.key.keysym.sym)
        {
        case SDLK_ESCAPE:
            data->key = LV_KEY_ESC;
            break;
        case SDLK_BACKSPACE:
            data->key = LV_KEY_BACKSPACE;
            break;
        case SDLK_HOME:
            data->key = LV_KEY_HOME;
            break;
        case SDLK_RETURN:
            data->key = LV_KEY_ENTER;
            break;
        case SDLK_PAGEDOWN:
            data->key = LV_KEY_PREV;
            break;
        case SDLK_TAB:
            data->key = LV_KEY_NEXT;
            break;
        case SDLK_PAGEUP:
            data->key = LV_KEY_NEXT;
            break;
        case SDLK_UP:
            data->key = LV_KEY_UP;
            break;
        case SDLK_DOWN:
            data->key = LV_KEY_DOWN;
            break;
        case SDLK_LEFT:
            data->key = LV_KEY_LEFT;
            break;
        case SDLK_RIGHT:
            data->key = LV_KEY_RIGHT;
            break;
        }
    }
}

lv_indev_t *lv_sdl_init_key_input(void)
{
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = sdl_input_read;

    return lv_indev_drv_register(&indev_drv);
}

void lv_sdl_deinit_key_input(void)
{
}
