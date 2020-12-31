#pragma once

#include <gst/gst.h>
#include <gst/app/app.h>

int gst_sample_initialize();
int gst_sample_finalize();

static void audioEos(GstAppSink *appsink, void *userData);
static GstFlowReturn audioNewPreroll(GstAppSink *appsink, void *userData);
static GstFlowReturn audioNewSample(GstAppSink *appsink, void *userData);
static void videoEos(GstAppSink *appsink, void *userData);
static GstFlowReturn videoNewPreroll(GstAppSink *appsink, void *userData);
static GstFlowReturn videoNewSample(GstAppSink *appsink, void *userData);