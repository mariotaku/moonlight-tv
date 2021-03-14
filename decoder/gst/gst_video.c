#include "video.h"

#include <stdio.h>

#include <gst/gst.h>
#include <gst/app/app.h>

static GstElement *pipeline, *video_source;

static void vsink_eos(GstAppSink *appsink, gpointer user_data);
static GstFlowReturn vsink_preroll(GstAppSink *appsink, gpointer user_data);
static GstFlowReturn vsink_newsample(GstAppSink *appsink, gpointer user_data);

static int _setup(int videoFormat, int width, int height, int redrawRate, void *context, int drFlags)
{
    pipeline = gst_parse_launch("appsrc name=vsrc ! h264parse ! appsink name=vsnk", NULL);

    if (!pipeline)
        return -1;

    GstElement *vsrc, *vsnk;
    vsrc = gst_bin_get_by_name(GST_BIN(pipeline), "vsrc");
    g_assert(vsrc);
    vsnk = gst_bin_get_by_name(GST_BIN(pipeline), "vsnk");
    g_assert(vsnk);

    GstAppSinkCallbacks vcb = {
        .eos = vsink_eos,
        .new_preroll = vsink_preroll,
        .new_sample = vsink_newsample,
    };
    gst_app_sink_set_callbacks(GST_APP_SINK(vsnk), &vcb, NULL, NULL);

    gst_app_src_set_stream_type(GST_APP_SRC(vsrc), GST_APP_STREAM_TYPE_STREAM);

    video_source = vsrc;

    if (gst_element_set_state(pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE)
        return -1;

    return 0;
}

static void _cleanup()
{
    if (pipeline)
    {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        video_source = NULL;
        pipeline = NULL;
    }
}

static int _submit_decode_unit(PDECODE_UNIT decodeUnit)
{
    GstBuffer *buf = gst_buffer_new();
    for (PLENTRY entry = decodeUnit->bufferList; entry != NULL; entry = entry->next)
    {
        GstMemory *mem = gst_memory_new_wrapped(GST_MEMORY_FLAG_READONLY, entry->data, entry->length, 0, entry->length, NULL, NULL);
        gst_buffer_append_memory(buf, mem);
    }
    if (gst_app_src_push_buffer(GST_APP_SRC(video_source), buf) != 0)
        return DR_NEED_IDR;
    return DR_OK;
}

void vsink_eos(GstAppSink *appsink, gpointer user_data)
{
}

GstFlowReturn vsink_preroll(GstAppSink *appsink, gpointer user_data)
{
    GstSample *preroll = gst_app_sink_pull_preroll(appsink);
    GstCaps *caps = gst_sample_get_caps(preroll);
    GstStructure *cap = gst_caps_get_structure(caps, 0);
    g_message(gst_caps_to_string(caps));
    int width, height;
    g_assert(gst_structure_get_int(cap, "width", &width));
    g_assert(gst_structure_get_int(cap, "height", &height));

    gst_sample_unref(preroll);

    printf("video width=%d, height=%d\n", width, height);
    return GST_FLOW_OK;
}

GstFlowReturn vsink_newsample(GstAppSink *appsink, gpointer user_data)
{
    GstSample *sample = gst_app_sink_pull_sample(appsink);

    GstBuffer *buf = gst_sample_get_buffer(sample);

    GstMapInfo info;
    gst_buffer_map(buf, &info, GST_MAP_READ);

    gst_buffer_unmap(buf, &info);
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

DECODER_RENDERER_CALLBACKS decoder_callbacks_gst = {
    .setup = _setup,
    .cleanup = _cleanup,
    .submitDecodeUnit = _submit_decode_unit,
    .capabilities = CAPABILITY_SLICES_PER_FRAME(4) | CAPABILITY_REFERENCE_FRAME_INVALIDATION_AVC | CAPABILITY_DIRECT_SUBMIT,
};
