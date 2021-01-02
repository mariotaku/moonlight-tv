
#include "src/connection.h"
#include "src/config.h"
#include "src/platform.h"
#include "src/input/sdl.h"

#include <Limelight.h>

#include "libgamestream/client.h"
#include "libgamestream/errors.h"

#include "stream/audio/audio.h"
#include "stream/video/video.h"

static bool session_running;

static void stream(PSERVER_DATA server, PCONFIGURATION config, enum platform system, int appId)
{

    if (appId < 0)
    {
        fprintf(stderr, "Can't find app %s\n", config->app);
        exit(-1);
    }

    int gamepads = 0;
    gamepads += sdl_gamepads;
    int gamepad_mask = 0;
    for (int i = 0; i < gamepads && i < 4; i++)
        gamepad_mask = (gamepad_mask << 1) + 1;

    int ret = gs_start_app(server, &config->stream, appId, config->sops, config->localaudio, gamepad_mask);
    if (ret < 0)
    {
        if (ret == GS_NOT_SUPPORTED_4K)
            fprintf(stderr, "Server doesn't support 4K\n");
        else if (ret == GS_NOT_SUPPORTED_MODE)
            fprintf(stderr, "Server doesn't support %dx%d (%d fps) or try --unsupported option\n", config->stream.width, config->stream.height, config->stream.fps);
        else if (ret == GS_NOT_SUPPORTED_SOPS_RESOLUTION)
            fprintf(stderr, "SOPS isn't supported for the resolution %dx%d, use supported resolution or add --nosops option\n", config->stream.width, config->stream.height);
        else if (ret == GS_ERROR)
            fprintf(stderr, "Gamestream error: %s\n", gs_error);
        else
            fprintf(stderr, "Errorcode starting app: %d\n", ret);
        exit(-1);
    }

    int drFlags = 0;

    if (config->debug_level > 0)
    {
        printf("Stream %d x %d, %d fps, %d kbps\n", config->stream.width, config->stream.height, config->stream.fps, config->stream.bitrate);
    }

    if (IS_EMBEDDED(system))
        loop_init();

    platform_start(system);
    LiStartConnection(&server->serverInfo, &config->stream, &connection_callbacks, platform_get_video(system), platform_get_audio(system, config->audio_device), NULL, drFlags, config->audio_device, 0);
    session_running = true;

    while (session_running)
    {
        // Wait until interrupted
    }

    LiStopConnection();

    if (config->quitappafter)
    {
        if (config->debug_level > 0)
            printf("Sending app quit request ...\n");
        gs_quit_app(server);
    }

    platform_stop(system);
}