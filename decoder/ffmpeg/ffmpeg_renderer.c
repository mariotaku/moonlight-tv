#include <stddef.h>
#include <stdio.h>
#include <assert.h>

#include <Limelight.h>
#include <pthread.h>

#include "common.h"

#include <libavcodec/avcodec.h>
#include <SDL.h>

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
    if (!renderer_ready || !frame) {
        return false;
    }
    if (!frame_texture) {
        frame_texture = SDL_CreateTexture(host_render_context_ffmpeg->renderer, SDL_PIXELFORMAT_YV12,
                                          SDL_TEXTUREACCESS_STREAMING, width, height);
    }
    switch (frame->format) {
        case AV_PIX_FMT_YUV420P:
            SDL_UpdateYUVTexture(frame_texture, NULL, frame->data[0], frame->linesize[0],
                                 frame->data[1], frame->linesize[1],
                                 frame->data[2], frame->linesize[2]);
            break;
//            case AV_PIX_FMT_NV12:
//                SDL_UpdateYUVTexture(m_Texture, NULL, frame->data[0], frame->linesize[0], frame->data[1],
//                                     frame->linesize[1],
//                                     frame->data[2], frame->linesize[2];
//                break;
        default:
            // TODO: handle unsupported pixel format
            return false;
    }
    frame_arrived |= true;
    return true;
}

static bool renderer_draw() {
    if (!frame_arrived || !renderer_ready) {
        return false;
    }
    SDL_RenderCopy(host_render_context_ffmpeg->renderer, frame_texture, NULL, NULL);
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