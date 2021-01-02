#include "demo.h"
#include "main.h"

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

#include <NDL_directmedia.h>
#include <SDL.h>

static GstElement *pipeline;
static GstBus *bus;

static void cb_message(GstBus *bus, GstMessage *message, gpointer user_data);

static void audioEos(GstAppSink *appsink, void *userData);
static GstFlowReturn audioNewPreroll(GstAppSink *appsink, void *userData);
static GstFlowReturn audioNewSample(GstAppSink *appsink, void *userData);
static void videoEos(GstAppSink *appsink, void *userData);
static GstFlowReturn videoNewPreroll(GstAppSink *appsink, void *userData);
static GstFlowReturn videoNewSample(GstAppSink *appsink, void *userData);

int gst_demo_initialize()
{
    GstStateChangeReturn ret;

    GstElement *audiosink, *videosink;
    pipeline = gst_parse_launch("curlhttpsrc location=http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4 ! qtdemux name=demux \
    demux.audio_0 ! queue ! aacparse ! audio/mpeg,mpegversion=4,stream-format=adts ! appsink name=audsink \
    demux.video_0 ! queue ! h264parse config-interval=-1 ! video/x-h264,stream-format=byte-stream,alignment=nal ! appsink name=vidsink",
                                NULL);

    g_assert(pipeline);

    audiosink = gst_bin_get_by_name(GST_BIN(pipeline), "audsink");
    g_assert(audiosink);

    videosink = gst_bin_get_by_name(GST_BIN(pipeline), "vidsink");
    g_assert(videosink);

    GstAppSinkCallbacks audioCallbacks = {
        .eos = audioEos,
        .new_preroll = audioNewPreroll,
        .new_sample = audioNewSample,
    };
    gst_app_sink_set_callbacks(GST_APP_SINK(audiosink), &audioCallbacks, NULL, NULL);

    GstAppSinkCallbacks videoCallbacks = {
        .eos = videoEos,
        .new_preroll = videoNewPreroll,
        .new_sample = videoNewSample,
    };
    gst_app_sink_set_callbacks(GST_APP_SINK(videosink), &videoCallbacks, NULL, NULL);

    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    gst_bus_add_signal_watch(bus);
    g_signal_connect(bus, "message", (GCallback)cb_message, pipeline);

    /* Start playing */
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    return 0;
}

int gst_demo_finalize()
{
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}

bool gst_demo_dispatch_event(SDL_Event ev)
{
    return false;
}

bool gst_demo_render_background()
{
    return false;
}

void audioEos(GstAppSink *appsink, gpointer user_data)
{
    NDL_DirectAudioClose();
}

GstFlowReturn audioNewPreroll(GstAppSink *appsink, gpointer user_data)
{
    GstSample *preroll = gst_app_sink_pull_preroll(appsink);
    GstCaps *caps = gst_sample_get_caps(preroll);
    GstStructure *cap = gst_caps_get_structure(caps, 0);
    int channels = 0, rate = 0;
    gst_structure_get_int(cap, "channels", &channels);
    gst_structure_get_int(cap, "rate", &rate);

    gst_sample_unref(preroll);

    NDL_DIRECTAUDIO_DATA_INFO info = {
        .numChannel = channels,
        .bitPerSample = 16,
        .nodelay = 1,
        .upperThreshold = 48,
        .lowerThreshold = 16,
        .channel = NDL_DIRECTAUDIO_CH_MAIN,
        .srcType = NDL_DIRECTAUDIO_SRC_TYPE_AAC,
        .samplingFreq = NDL_DIRECTAUDIO_SAMPLING_FREQ_OF(rate)};
    if (NDL_DirectAudioOpen(&info) == -1)
    {
        return GST_FLOW_ERROR;
    }
    return GST_FLOW_OK;
}

GstFlowReturn audioNewSample(GstAppSink *appsink, gpointer user_data)
{
    GstSample *sample = gst_app_sink_pull_sample(appsink);

    GstBuffer *buf = gst_sample_get_buffer(sample);

    GstMapInfo info;
    gst_buffer_map(buf, &info, GST_MAP_READ);

    NDL_DirectAudioPlay(info.data, info.size);

    gst_buffer_unmap(buf, &info);
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

void videoEos(GstAppSink *appsink, gpointer user_data)
{
    NDL_DirectVideoClose();
}

GstFlowReturn videoNewPreroll(GstAppSink *appsink, gpointer user_data)
{
    GstSample *preroll = gst_app_sink_pull_preroll(appsink);
    GstCaps *caps = gst_sample_get_caps(preroll);
    GstStructure *cap = gst_caps_get_structure(caps, 0);

    int width, height;
    g_assert(gst_structure_get_int(cap, "width", &width));
    g_assert(gst_structure_get_int(cap, "height", &height));

    gst_sample_unref(preroll);

    NDL_DIRECTVIDEO_DATA_INFO info = {width, height};
    if (NDL_DirectVideoOpen(&info) == -1)
    {
        return GST_FLOW_ERROR;
    }
    return GST_FLOW_OK;
}

GstFlowReturn videoNewSample(GstAppSink *appsink, gpointer user_data)
{
    GstSample *sample = gst_app_sink_pull_sample(appsink);

    GstBuffer *buf = gst_sample_get_buffer(sample);

    GstMapInfo info;
    gst_buffer_map(buf, &info, GST_MAP_READ);

    NDL_DirectVideoPlay(info.data, info.size);

    gst_buffer_unmap(buf, &info);
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

/* called when a new message is posted on the bus */
void cb_message(GstBus *bus, GstMessage *message, gpointer user_data)
{
    GstElement *pipeline = GST_ELEMENT(user_data);

    switch (GST_MESSAGE_TYPE(message))
    {
    case GST_MESSAGE_ERROR:
        g_print("we received an error!\n");
        request_exit();
        break;
    case GST_MESSAGE_EOS:
        g_print("we reached EOS\n");
        request_exit();
        break;
    default:
        break;
    }
}