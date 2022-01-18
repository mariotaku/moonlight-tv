#include "ndl_common.h"
#include "module/logging.h"

#include <stdlib.h>
#include <string.h>

#include <Limelight.h>
#include <NDL_directmedia_v2.h>

// 2MB decode size should be fairly enough for everything
#define DECODER_BUFFER_SIZE 2048 * 1024

#define decoder_callbacks PLUGIN_SYMBOL_NAME(decoder_callbacks)

static char *ndl_buffer;

static int ndl_setup(int videoFormat, int width, int height, int redrawRate, void *context, int drFlags) {
    switch (videoFormat) {
        case VIDEO_FORMAT_H264:
            media_info.video.type = NDL_VIDEO_TYPE_H264;
            break;
        case VIDEO_FORMAT_H265:
            media_info.video.type = NDL_VIDEO_TYPE_H265;
            hdr_info.supported = false;
            break;
        case VIDEO_FORMAT_H265_MAIN10:
            media_info.video.type = NDL_VIDEO_TYPE_H265;
            hdr_info.supported = true;
            hdr_info.value.displayPrimariesX0 = 13250;
            hdr_info.value.displayPrimariesY0 = 34500;
            hdr_info.value.displayPrimariesX1 = 7500;
            hdr_info.value.displayPrimariesY1 = 3000;
            hdr_info.value.displayPrimariesX2 = 34000;
            hdr_info.value.displayPrimariesY2 = 16000;
            hdr_info.value.whitePointX = 15635;
            hdr_info.value.whitePointY = 16450;
            hdr_info.value.maxDisplayMasteringLuminance = 1000;
            hdr_info.value.minDisplayMasteringLuminance = 50;
            hdr_info.value.maxContentLightLevel = 1000;
            hdr_info.value.maxPicAverageLightLevel = 400;
            break;
        default:
            return ERROR_UNKNOWN_CODEC;
    }
    media_info.video.width = width;
    media_info.video.height = height;
    media_info.video.unknown1 = 0;
    if (media_reload() != 0) {
        applog_e("NDL", "NDL_DirectMediaLoad failed: %s", NDL_DirectMediaGetError());
        return ERROR_DECODER_OPEN_FAILED;
    }
    ndl_buffer = malloc(DECODER_BUFFER_SIZE);
    if (ndl_buffer == NULL) {
        applog_f("NDL", "Not enough memory");
        media_unload();
        return ERROR_OUT_OF_MEMORY;
    }
    return 0;
}

static void ndl_cleanup() {
    media_unload();
    memset(&media_info.video, 0, sizeof(media_info.video));
    memset(&hdr_info, 0, sizeof(hdr_info));
    if (ndl_buffer) {
        free(ndl_buffer);
    }
}

static int ndl_submit_decode_unit(PDECODE_UNIT decodeUnit) {
    if (!media_loaded)
        return DR_NEED_IDR;
    if (decodeUnit->fullLength < DECODER_BUFFER_SIZE) {
        int length = 0;
        for (PLENTRY entry = decodeUnit->bufferList; entry != NULL; entry = entry->next) {
            memcpy(ndl_buffer + length, entry->data, entry->length);
            length += entry->length;
        }

        if (NDL_DirectVideoPlay(ndl_buffer, length, 0) != 0) {
            applog_w("NDL", "NDL_DirectVideoPlay failed");
            return DR_NEED_IDR;
        }
    } else {
        applog_w("NDL", "Video decode buffer too small, skip this frame");
        return DR_NEED_IDR;
    }

    return DR_OK;
}

DECODER_RENDERER_CALLBACKS decoder_callbacks = {
        .setup = ndl_setup,
        .cleanup = ndl_cleanup,
        .submitDecodeUnit = ndl_submit_decode_unit,
        .capabilities = CAPABILITY_SLICES_PER_FRAME(4) | CAPABILITY_DIRECT_SUBMIT,
};
