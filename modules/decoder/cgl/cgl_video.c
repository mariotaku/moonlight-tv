#include <stdlib.h>
#include <string.h>

#include <Limelight.h>
#include <cgl.h>

#include "module/api.h"
#include "module/logging.h"

// 2MB decode size should be fairly enough for everything
#define DECODER_BUFFER_SIZE 2048 * 1024

static char *cgl_buffer;

static int vid_setup(int videoFormat, int width, int height, int redrawRate, void *context, int drFlags) {
    CGL_VIDEO_INFO_T info = {
            .width = width,
            .height = height,
            .source = CGL_VIDEO_SOURCE_MAIN,
            .tridType = CGL_VIDEO_3D_TYPE_NONE,
    };
    if (CGL_OpenVideo(&info) != 0) {
        applog_e("CGL", "Couldn't initialize video decoding");
        return ERROR_DECODER_OPEN_FAILED;
    }
    cgl_buffer = malloc(DECODER_BUFFER_SIZE);
    if (cgl_buffer == NULL) {
        applog_f("CGL", "Not enough memory");
        CGL_CloseVideo();
        return ERROR_OUT_OF_MEMORY;
    }

    return 0;
}

static void vid_cleanup() {
    CGL_CloseVideo();
    if (cgl_buffer) {
        free(cgl_buffer);
    }
}

static int vid_submit(PDECODE_UNIT decodeUnit) {
    if (decodeUnit->fullLength < DECODER_BUFFER_SIZE) {
        int length = 0;
        for (PLENTRY entry = decodeUnit->bufferList; entry != NULL; entry = entry->next) {
            memcpy(cgl_buffer + length, entry->data, entry->length);
            length += entry->length;
        }
        if (CGL_PlayVideo(cgl_buffer, length) != 0) {
            applog_w("CGL", "CGL_PlayVideo returned non zero");
            return DR_NEED_IDR;
        }
    } else {
        applog_w("CGL", "Video decode buffer too small, skip this frame");
        return DR_NEED_IDR;
    }

    return DR_OK;
}

MODULE_API DECODER_RENDERER_CALLBACKS decoder_callbacks_cgl = {
        .setup = vid_setup,
        .cleanup = vid_cleanup,
        .submitDecodeUnit = vid_submit,
        .capabilities = CAPABILITY_SLICES_PER_FRAME(4) | CAPABILITY_DIRECT_SUBMIT,
};
