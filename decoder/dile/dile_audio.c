#include "audio.h"

#include <stdio.h>
#include <opus_multistream.h>

#include <pbnjson.h>

#include <ResourceManagerClient_c.h>
#include <dile_audio_direct.h>

#include "media_services.h"
#include "utils.h"

static OpusMSDecoder *decoder;
static short pcmBuffer[FRAME_SIZE * MAX_CHANNEL_COUNT];
static int channelCount;

static unsigned char *dile_buffer = NULL;
static ResourceManagerClientHandle *rmhandle = NULL;
static jvalue_ref acquired_resources = NULL;
static MEDIA_SERVICES_HANDLE adec_services;

static bool first_sample_arrived = false;

static bool policyActionHandler(const char *action, const char *resources,
                                const char *requestor_type, const char *requestor_name,
                                const char *connection_id);

static int _init(int audioConfiguration, POPUS_MULTISTREAM_CONFIGURATION opusConfig, void *context, int arFlags)
{
  int rc;
  decoder = opus_multistream_decoder_create(opusConfig->sampleRate, opusConfig->channelCount, opusConfig->streams, opusConfig->coupledStreams, opusConfig->mapping, &rc);

  channelCount = opusConfig->channelCount;

  if (!(rmhandle = ResourceManagerClientCreate(NULL, policyActionHandler)))
  {
    return -1;
  }
  if (!ResourceManagerClientRegisterPipeline(rmhandle, "direct_av"))
  {
    return false;
  }
  const char *connId = ResourceManagerClientGetConnectionID(rmhandle);
  const char *appId = getenv("APPID");

  ResourceManagerClientNotifyForeground(rmhandle);

  jvalue_ref resreq = NULL;
  char *resresp = NULL;
  MRCResourceList resList = MRCCalcAdecResources(kAudioPCM, 0, channelCount);
  resreq = serialize_resource_aquire_req(resList);
  ResourceManagerClientAcquire(rmhandle, jvalue_stringify(resreq), &resresp);
  MRCDeleteResourceList(resList);
  acquired_resources = parse_resource_aquire_resp(resresp);
  j_release(&resreq);
  free(resresp);

  adec_services = media_services_connect(connId, appId, acquired_resources, MEDIA_SERVICES_TYPE_AUDIO);

  media_services_set_audio_data(adec_services, connId);

  if (DILE_AUDIO_DIRECT_Open(0, 11 /* Warning: magic number! */) != 0)
  {
    printf("Failed to open audio\n");
    return -1;
  }
  if (DILE_AUDIO_DIRECT_SetNoDelayParam(0, 1, 48, 16) != 0)
  {
    printf("Failed to set low delay param\n");
    return -1;
  }
  if (DILE_AUDIO_DIRECT_Start(0, DILE_AUDIO_DIRECT_SRCTYPE_PCM, DILE_AUDIO_DIRECT_SAMPFREQ_OF(opusConfig->sampleRate),
                              channelCount, 16) != 0)
  {
    printf("Failed to start audio\n");
    DILE_AUDIO_DIRECT_Close(0);
    return -1;
  }

  return 0;
}

static void _cleanup()
{
  if (decoder != NULL)
    opus_multistream_decoder_destroy(decoder);

  DILE_AUDIO_DIRECT_Stop(0);
  DILE_AUDIO_DIRECT_Close(0);

  if (rmhandle)
  {
    const char *connId = ResourceManagerClientGetConnectionID(rmhandle);

    media_services_disconnect(adec_services, connId);

    if (acquired_resources)
    {
      ResourceManagerClientRelease(rmhandle, jvalue_stringify(jobject_get(acquired_resources, J_CSTR_TO_BUF("resources"))));
      j_release(&acquired_resources);
      acquired_resources = NULL;
    }

    ResourceManagerClientUnregisterPipeline(rmhandle);

    ResourceManagerClientDestroy(rmhandle);
  }
}

static void _feed(char *data, int length)
{
  int decodeLen = opus_multistream_decode(decoder, data, length, pcmBuffer, FRAME_SIZE, 0);
  if (decodeLen > 0)
  {
    if (DILE_AUDIO_DIRECT_Write(0, pcmBuffer, decodeLen * channelCount * sizeof(short)) != 0)
    {
      fprintf(stderr, "DILE_AUDIO_DIRECT_Write Failed\n");
    }
    else if (!first_sample_arrived && rmhandle)
    {
      ResourceManagerClientNotifyActivity(rmhandle);
      media_services_feed_arrived(adec_services);
      first_sample_arrived = true;
    }
  }
  else
  {
    printf("Opus error from decode: %d\n", decodeLen);
  }
}

AUDIO_RENDERER_CALLBACKS PLUGIN_SYMBOL_NAME(audio_callbacks) = {
    .init = _init,
    .cleanup = _cleanup,
    .decodeAndPlaySample = _feed,
    .capabilities = CAPABILITY_DIRECT_SUBMIT,
};

bool policyActionHandler(const char *action, const char *resources,
                         const char *requestor_type, const char *requestor_name,
                         const char *connection_id)
{
  printf("POLICY_ACTION: action=%s, resources=%s, requestor_type=%s,"
         "requestor_name=%s, connection_id=%s\n",
         action, resources, requestor_type, requestor_name, connection_id);
  return true;
}
