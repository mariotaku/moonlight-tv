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
#include "lv_sdl_drv_pointer_input.h"

static void sdl_input_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void)drv;

    SDL_PumpEvents();
    static SDL_Event e;
    data->continue_reading = SDL_PeepEvents(&e, 1, SDL_GETEVENT, SDL_MOUSEMOTION, SDL_MOUSEBUTTONUP) > 0;
    static lv_indev_state_t state = LV_INDEV_STATE_REL;
    if (e.type == SDL_MOUSEBUTTONDOWN)
    {
        state = LV_INDEV_STATE_PR;
        data->point = (lv_point_t){.x = e.button.x, .y = e.button.y};
    }
    else if (e.type == SDL_MOUSEBUTTONUP)
    {
        state = LV_INDEV_STATE_REL;
        data->point = (lv_point_t){.x = e.button.x, .y = e.button.y};
    }
    else
    {
        data->point = (lv_point_t){.x = e.motion.x, .y = e.motion.y};
    }
    data->state = state;
}

lv_indev_t *lv_sdl_init_pointer_input(void)
{
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = sdl_input_read;

    return lv_indev_drv_register(&indev_drv);
}

void lv_sdl_deinit_pointer_input(void)
{
}
