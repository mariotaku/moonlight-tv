#include <Limelight.h>

static int noop_setup(int videoFormat, int width, int height, int redrawRate, void *context, int drFlags)
{
    return 0;
}

static void noop_cleanup()
{
}

static int noop_submit_decode_unit(PDECODE_UNIT decodeUnit)
{
    return DR_OK;
}

DECODER_RENDERER_CALLBACKS decoder_callbacks_dummy = {
    .setup = noop_setup,
    .cleanup = noop_cleanup,
    .submitDecodeUnit = noop_submit_decode_unit,
    .capabilities = CAPABILITY_SLICES_PER_FRAME(4) | CAPABILITY_DIRECT_SUBMIT,
};
