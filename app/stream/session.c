#include "session.h"
#include "settings.h"

#include "connection.h"
#include "platform.h"
// Include source directly in order to use static functions
#include "input/sdl.c"
#include "sdl/user_event.h"

#include <Limelight.h>

#include <SDL.h>
#include "libgamestream/client.h"
#include "libgamestream/errors.h"

#include "backend/computer_manager.h"

STREAMING_STATUS streaming_status = STREAMING_NONE;
int streaming_errno = GS_OK;

SDL_bool session_running = SDL_FALSE;
SDL_Thread *streaming_thread;
SDL_mutex *lock;
SDL_cond *cond;

SDL_Cursor *_blank_cursor;

short streaming_display_width, streaming_display_height;
bool streaming_quitapp_requested;
bool streaming_no_control;

typedef struct
{
    SERVER_DATA *server;
    CONFIGURATION *config;
    int appId;
} STREAMING_REQUEST;

static int _streaming_thread_action(STREAMING_REQUEST *req);

static void release_gamecontroller_buttons(SDL_Event ev);

void streaming_init()
{
    streaming_thread = NULL;
    streaming_status = STREAMING_NONE;
    streaming_quitapp_requested = false;
    lock = SDL_CreateMutex();
    cond = SDL_CreateCond();
    int32_t cursorData[2] = {0, 0};
    _blank_cursor = SDL_CreateCursor((Uint8 *)cursorData, (Uint8 *)cursorData, 8, 8, 4, 4);

    sdlinput_init("assets/gamecontrollerdb.txt");
}

void streaming_destroy()
{
    streaming_interrupt(streaming_quitapp_requested);
    streaming_wait_for_stop();

    SDL_DestroyCond(cond);
    SDL_DestroyMutex(lock);
}

void streaming_begin(PSERVER_DATA server, int app_id)
{
    PCONFIGURATION config = settings_load();

    STREAMING_REQUEST *req = malloc(sizeof(STREAMING_REQUEST));
    req->server = server;
    req->config = config;
    req->appId = app_id;
    streaming_thread = SDL_CreateThread((SDL_ThreadFunction)_streaming_thread_action, "streaming", req);
}

void streaming_interrupt(bool quitapp)
{
    SDL_LockMutex(lock);
    streaming_quitapp_requested = quitapp;
    session_running = SDL_FALSE;
    SDL_CondSignal(cond);
    SDL_UnlockMutex(lock);
}

void streaming_wait_for_stop()
{
    if (streaming_thread == NULL)
    {
        return;
    }
    SDL_WaitThread(streaming_thread, NULL);
    streaming_thread = NULL;
}

bool streaming_running()
{
    return session_running == SDL_TRUE;
}

static bool nocontrol_handle_event(SDL_Event ev)
{
    switch (ev.type)
    {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
    {
        int modifier = 0;
        switch (ev.key.keysym.sym)
        {
        case SDLK_RSHIFT:
        case SDLK_LSHIFT:
            modifier = MODIFIER_SHIFT;
            break;
        case SDLK_RALT:
        case SDLK_LALT:
            modifier = MODIFIER_ALT;
            break;
        case SDLK_RCTRL:
        case SDLK_LCTRL:
            modifier = MODIFIER_CTRL;
            break;
        }

        if (modifier != 0)
        {
            if (ev.type == SDL_KEYDOWN)
            {
                keyboard_modifiers |= modifier;
            }
            else
            {
                keyboard_modifiers &= ~modifier;
            }
        }

        // Quit the stream if all the required quit keys are down
        if ((keyboard_modifiers & ACTION_MODIFIERS) == ACTION_MODIFIERS && ev.key.keysym.sym == QUIT_KEY && ev.type == SDL_KEYUP)
        {
            return SDL_QUIT_APPLICATION;
        }
        else if ((keyboard_modifiers & ACTION_MODIFIERS) == ACTION_MODIFIERS && ev.key.keysym.sym == FULLSCREEN_KEY && ev.type == SDL_KEYUP)
        {
            return SDL_TOGGLE_FULLSCREEN;
        }
        else if ((keyboard_modifiers & ACTION_MODIFIERS) == ACTION_MODIFIERS)
        {
            return SDL_MOUSE_UNGRAB;
        }
        break;
    }
    default:
        break;
    }
    return SDL_NOTHING;
}

bool streaming_dispatch_event(SDL_Event ev)
{
    if (streaming_status != STREAMING_STREAMING)
    {
        return false;
    }
    // Don't mess with Magic Remote yet
    // TODO https://github.com/mariotaku/moonlight-sdl/issues/1
    // TODO https://github.com/mariotaku/moonlight-sdl/issues/2
    if (!streaming_no_control && ev.type == SDL_MOUSEMOTION)
    {
        LiSendMousePositionEvent(ev.motion.x, ev.motion.y, streaming_display_width, streaming_display_height);
        return false;
    }
    switch (streaming_no_control ? nocontrol_handle_event(ev) : sdlinput_handle_event(&ev))
    {
    case SDL_MOUSE_GRAB:
        SDL_SetCursor(_blank_cursor);
        break;
    case SDL_MOUSE_UNGRAB:
        SDL_SetCursor(NULL);
        break;
    case SDL_QUIT_APPLICATION:
    {
        if (ev.type == SDL_CONTROLLERBUTTONDOWN || ev.type == SDL_CONTROLLERBUTTONUP)
        {
            // Put gamepad to neutral state
            release_gamecontroller_buttons(ev);
        }

        SDL_Event quitapp;
        quitapp.type = SDL_USEREVENT;
        quitapp.user.code = SDL_USER_ST_QUITAPP_CONFIRM;
        SDL_PushEvent(&quitapp);
        break;
    }
    default:
        break;
    }
    return false;
}

void streaming_display_size(short width, short height)
{
    streaming_display_width = width;
    streaming_display_height = height;
}

int _streaming_thread_action(STREAMING_REQUEST *req)
{
    streaming_status = STREAMING_CONNECTING;
    streaming_errno = GS_OK;
    PSERVER_DATA server = req->server;
    PCONFIGURATION config = req->config;
    streaming_no_control = config->viewonly;
    int appId = req->appId;

    int gamepads = sdl_gamepads;
    int gamepad_mask = 0;
    for (int i = 0; i < gamepads && i < 4; i++)
        gamepad_mask = (gamepad_mask << 1) + 1;

    if (config->debug_level > 0)
    {
        printf("Launch app %d...\n", appId);
    }
    int ret = gs_start_app(server, &config->stream, appId, config->sops, config->localaudio, gamepad_mask);
    if (ret < 0)
    {
        streaming_status = STREAMING_ERROR;
        streaming_errno = ret;
        free(req);
        return -1;
    }

    int drFlags = 0;

    if (config->debug_level > 0)
    {
        printf("Stream %d x %d, %d fps, %d kbps\n", config->stream.width, config->stream.height, config->stream.fps, config->stream.bitrate);
    }

    int startResult = LiStartConnection(&server->serverInfo, &config->stream, &connection_callbacks,
                                        platform_get_video(NONE), platform_get_audio(NONE, config->audio_device), NULL, drFlags, config->audio_device, 0);
    if (startResult != 0)
    {
        streaming_status = STREAMING_ERROR;
        streaming_errno = GS_WRONG_STATE;
        goto thread_cleanup;
    }
    session_running = true;
    streaming_status = STREAMING_STREAMING;
    rumble_handler = sdlinput_rumble;

    SDL_LockMutex(lock);
    while (session_running)
    {
        // Wait until interrupted
        SDL_CondWait(cond, lock);
    }
    SDL_UnlockMutex(lock);

    rumble_handler = NULL;
    streaming_status = STREAMING_DISCONNECTING;
    LiStopConnection();

    if (config->quitappafter || streaming_quitapp_requested)
    {
        if (config->debug_level > 0)
            printf("Sending app quit request ...\n");
        gs_quit_app(server);
    }
    // TODO https://github.com/mariotaku/moonlight-sdl/issues/3
    streaming_status = STREAMING_NONE;

thread_cleanup:
    streaming_thread = NULL;

    free(req);
    return 0;
}

void release_gamecontroller_buttons(SDL_Event ev)
{
    PGAMEPAD_STATE gamepad;
    gamepad = get_gamepad(ev.cbutton.which);
    gamepad->buttons = 0;
    gamepad->leftTrigger = 0;
    gamepad->rightTrigger = 0;
    gamepad->leftStickX = 0;
    gamepad->leftStickY = 0;
    gamepad->rightStickX = 0;
    gamepad->rightStickY = 0;
    LiSendMultiControllerEvent(gamepad->id, activeGamepadMask, gamepad->buttons, gamepad->leftTrigger, gamepad->rightTrigger,
                               gamepad->leftStickX, gamepad->leftStickY, gamepad->rightStickX, gamepad->rightStickY);
}