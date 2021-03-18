#include "smp_common.h"

#include <stdbool.h>

#include "SDL_webOS.h"

StarfishDirectMediaPlayer *playerctx = NULL;
DirectMediaVideoConfig videoConfig = {DirectMediaVideoCodecUnknown, 0, 0};
DirectMediaAudioConfig audioConfig = {DirectMediaAudioCodecUnknown, 0, 0, 0};
static const char *windowId = NULL;
static bool playerOpened = false;
unsigned long long pts = 0;

void smp_player_create()
{
    playerOpened = false;
    playerctx = StarfishDirectMediaPlayer_Create("com.limelight.webos");
    windowId = SDL_webOSCreateExportedWindow(0);

    SDL_Rect src = {0, 0, videoConfig.width, videoConfig.height};
    SDL_Rect dst = {0, 0, 1920, 1080};
    SDL_webOSSetExportedWindow(windowId, &src, &dst);
}

void smp_player_destroy()
{
    if (playerctx)
    {
        StarfishDirectMediaPlayer_Destroy(playerctx);
        playerctx = NULL;
    }
    if (windowId)
    {
        SDL_webOSDestroyExportedWindow(windowId);
        windowId = NULL;
    }
}
void smp_player_open()
{
    if (playerOpened)
        return;
    if (windowId && audioConfig.codec != DirectMediaAudioCodecUnknown && videoConfig.codec != DirectMediaVideoCodecUnknown)
    {
        StarfishDirectMediaPlayer_Open(playerctx, &audioConfig, &videoConfig, windowId);
        playerOpened = true;
    }
}

void smp_player_close()
{
    if (playerOpened)
    {
        StarfishDirectMediaPlayer_Close(playerctx);
        playerOpened = false;
    }
}