#include "ndl_common.h"
#include "stream/module/api.h"
#include "util/logging.h"

#include <stdlib.h>
#include <string.h>

#include <Limelight.h>
#include <NDL_directmedia.h>

// 2MB decode size should be fairly enough for everything
#define DECODER_BUFFER_SIZE 2048 * 1024

#define decoder_callbacks PLUGIN_SYMBOL_NAME(decoder_callbacks)

static char *ndl_buffer;

static int setup(int videoFormat, int width, int height, int redrawRate, void *context, int drFlags) {
    NDL_DIRECTVIDEO_DATA_INFO info = {width, height};
    if (NDL_DirectVideoOpen(&info) != 0) {
        applog_e("NDL", "NDL_DirectVideoOpen failed: %s", NDL_DirectMediaGetError());
        return ERROR_DECODER_OPEN_FAILED;
    }
    applog_d("NDL", "NDL_DirectVideoOpen %d * %d", width, height);
    ndl_buffer = malloc(DECODER_BUFFER_SIZE);
    if (ndl_buffer == NULL) {
        applog_f("NDL", "Not enough memory");

        NDL_DirectVideoClose();
        return ERROR_OUT_OF_MEMORY;
    }
    return 0;
}

static void cleanup() {
    NDL_DirectVideoClose();
    if (ndl_buffer) {
        free(ndl_buffer);
    }
}

static int submit(PDECODE_UNIT decodeUnit) {
    if (decodeUnit->fullLength < DECODER_BUFFER_SIZE) {
        int length = 0;
        for (PLENTRY entry = decodeUnit->bufferList; entry != NULL; entry = entry->next) {
            memcpy(ndl_buffer + length, entry->data, entry->length);
            length += entry->length;
        }

        if (NDL_DirectVideoPlay(ndl_buffer, length) != 0) {
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
        .setup = setup,
        .cleanup = cleanup,
        .submitDecodeUnit = submit,
        .capabilities = CAPABILITY_SLICES_PER_FRAME(4) | CAPABILITY_DIRECT_SUBMIT,
};
