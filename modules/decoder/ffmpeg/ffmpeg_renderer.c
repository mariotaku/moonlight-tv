#include <stddef.h>
#include <stdio.h>
#include <assert.h>

#include <Limelight.h>

#include "common.h"

#include <libavcodec/avcodec.h>
#include <SDL.h>
#include "util/logging.h"

static int width, height;

static bool renderer_ready = false, frame_arrived;
HOST_RENDER_CONTEXT *host_render_context_ffmpeg = NULL;
int render_current_frame_ffmpeg, render_next_frame_ffmpeg;
static SDL_Texture *frame_texture;

static bool renderer_setup(PSTREAM_CONFIGURATION conf, HOST_RENDER_CONTEXT *host_ctx) {
    renderer_ready = false;
    frame_arrived = false;
    width = conf->width;
    height = conf->height;
    render_current_frame_ffmpeg = render_next_frame_ffmpeg = 0;
    host_render_context_ffmpeg = host_ctx;

    frame_texture = NULL;

    renderer_ready = true;
    return true;
}

static bool renderer_submit_frame(AVFrame *frame) {
    if (!renderer_ready) {
        applog_w("FFMPEG", "Render is not ready.");
        return false;
    }
    if (!frame) {
        applog_w("FFMPEG", "Frame data is NULL.");
        return false;
    }
    if (!frame_texture) {
        SDL_PixelFormatEnum format;
        switch (frame->format) {
            case AV_PIX_FMT_YUV420P:
                format = SDL_PIXELFORMAT_YV12;
                break;
            default:
                return false;
        }
        frame_texture = SDL_CreateTexture(host_render_context_ffmpeg->renderer, format,
                                          SDL_TEXTUREACCESS_STREAMING, width, height);
    }
    SDL_UpdateYUVTexture(frame_texture, NULL, frame->data[0], frame->linesize[0],
                         frame->data[1], frame->linesize[1],
                         frame->data[2], frame->linesize[2]);
    frame_arrived |= true;
    return true;
}

static bool renderer_draw() {
    if (!frame_arrived || !renderer_ready) {
        return false;
    }
    HOST_RENDERER *renderer = host_render_context_ffmpeg->renderer;
    SDL_Rect viewport;
    SDL_RenderGetViewport(renderer, &viewport);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
    SDL_RenderClear(renderer);
    double srcratio = width / (double) height, dstratio = viewport.w / (double) viewport.h;
    SDL_Rect dstrect;
    if (srcratio > dstratio) {
        // Source is wider than destination
        dstrect.w = viewport.w;
        dstrect.h = viewport.w / srcratio;
        dstrect.x = 0;
        dstrect.y = (viewport.h - dstrect.h) / 2;
    } else {
        // Destination is wider than source
        dstrect.h = viewport.h;
        dstrect.w = viewport.h * srcratio;
        dstrect.y = 0;
        dstrect.x = (viewport.w - dstrect.w) / 2;
    }
    SDL_RenderCopy(renderer, frame_texture, NULL, &dstrect);
    return true;
}

static void renderer_cleanup() {
    renderer_ready = false;
    if (frame_texture) {
        SDL_DestroyTexture(frame_texture);
        frame_texture = NULL;
    }
    host_render_context_ffmpeg = NULL;
}

VIDEO_RENDER_CALLBACKS render_callbacks_ffmpeg = {
        .renderSetup = renderer_setup,
        .renderSubmit = (RenderSubmit) renderer_submit_frame,
        .renderDraw = renderer_draw,
        .renderCleanup = renderer_cleanup,
};