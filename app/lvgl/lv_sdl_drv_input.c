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
#include "lv_sdl_drv_input.h"

static quit_event_t quit_event = false;

static bool sdl_input_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void)drv;
    data->key = 0;

    static SDL_Event e;
    if (SDL_PollEvent(&e))
    {
        if (e.type == SDL_CONTROLLERBUTTONDOWN || e.type == SDL_KEYDOWN)
            data->state = LV_INDEV_STATE_PR;
        if (e.type == SDL_CONTROLLERBUTTONUP || e.type == SDL_KEYUP)
            data->state = LV_INDEV_STATE_REL;

        if (e.type == SDL_CONTROLLERDEVICEADDED)
            SDL_GameControllerOpen(e.cdevice.which);
        if (e.type == SDL_CONTROLLERDEVICEREMOVED)
            SDL_GameControllerClose(SDL_GameControllerFromInstanceID(e.cdevice.which));

        //Gamecontroller event
        if (e.type == SDL_CONTROLLERBUTTONDOWN || e.type == SDL_CONTROLLERBUTTONUP)
        {
            switch (e.cbutton.button)
            {
                case SDL_CONTROLLER_BUTTON_A:             data->key = LV_KEY_ENTER; break;
                case SDL_CONTROLLER_BUTTON_B:             data->key = LV_KEY_ESC; break;
                case SDL_CONTROLLER_BUTTON_X:             data->key = LV_KEY_BACKSPACE; break;
                case SDL_CONTROLLER_BUTTON_Y:             data->key = LV_KEY_HOME; break;
                case SDL_CONTROLLER_BUTTON_BACK:          data->key = LV_KEY_PREV; break;
                case SDL_CONTROLLER_BUTTON_START:         data->key = LV_KEY_NEXT; break;
                case SDL_CONTROLLER_BUTTON_LEFTSTICK:     data->key = LV_KEY_PREV; break;
                case SDL_CONTROLLER_BUTTON_RIGHTSTICK:    data->key = LV_KEY_NEXT; break;
                case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  data->key = LV_KEY_PREV; break;
                case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: data->key = LV_KEY_NEXT; break;
                case SDL_CONTROLLER_BUTTON_DPAD_UP:       data->key = LV_KEY_UP; break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:     data->key = LV_KEY_DOWN; break;
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:     data->key = LV_KEY_LEFT; break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:    data->key = LV_KEY_RIGHT; break;
            }
        }
        
        //Keyboard event
        if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
        {
            switch (e.key.keysym.sym)
            {
                case SDLK_ESCAPE:    data->key = LV_KEY_ESC; break;
                case SDLK_BACKSPACE: data->key = LV_KEY_BACKSPACE; break;
                case SDLK_HOME:      data->key = LV_KEY_HOME; break;
                case SDLK_RETURN:    data->key = LV_KEY_ENTER; break;
                case SDLK_PAGEDOWN:  data->key = LV_KEY_PREV; break;
                case SDLK_TAB:       data->key = LV_KEY_NEXT; break;
                case SDLK_PAGEUP:    data->key = LV_KEY_NEXT; break;
                case SDLK_UP:        data->key = LV_KEY_UP; break;
                case SDLK_DOWN:      data->key = LV_KEY_DOWN; break;
                case SDLK_LEFT:      data->key = LV_KEY_LEFT; break;
                case SDLK_RIGHT:     data->key = LV_KEY_RIGHT; break;
            }
        }

        //Quit event
        if((e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE) ||
            e.type == SDL_QUIT) {
            quit_event = true;
        }
    }

    if (SDL_PollEvent(NULL))
        return true; //There's more events to handle
    else
        return false;
}

quit_event_t get_quit_event(void)
{
    return quit_event;
}

void set_quit_event(quit_event_t quit)
{
    quit_event = quit;
}

lv_indev_t *lv_sdl_init_input(void)
{
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0)
        printf("SDL_InitSubSystem failed: %s\n", SDL_GetError());

    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    for (int i = 0; i < SDL_NumJoysticks(); i++)
    {
        SDL_GameControllerOpen(i);
    }

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = sdl_input_read;

    return lv_indev_drv_register(&indev_drv);
}

void lv_sdl_deinit_input(void)
{
    SDL_GameControllerClose(0);
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
}
