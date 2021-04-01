/*
 * This file is part of Moonlight Embedded.
 *
 * Copyright (C) 2015 Iwan Timmer
 *
 * Moonlight is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Moonlight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Moonlight; if not, see <http://www.gnu.org/licenses/>.
 */

#include "video.h"
#include "dile_platform.h"

#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Limelight.h>

#include <pbnjson.h>

#include <ResourceManagerClient_c.h>
#include <resource_calculator_c.h>
#include "VideoOutputService.h"
#include "VideoSinkManagerService.h"
#include <dile_vdec_direct.h>

#include "sps.h"

// 2MB decode size should be fairly enough for everything
#define DECODER_BUFFER_SIZE 2048 * 1024
#define FOURCC_H264 0x34363268
#define FOURCC_HEVC 0x43564548

static unsigned char *dile_buffer = NULL;
static ResourceManagerClientHandle *rmhandle = NULL;
static jvalue_ref acquired_resources = NULL;
static uint32_t video_fourcc = 0;

static bool policyActionHandler(const char *action, const char *resources,
                                const char *requestor_type, const char *requestor_name,
                                const char *connection_id);

static void playCallback(unsigned long long buffID);

static jvalue_ref serialize_resource_aquire_req(MRCResourceList list);
static jvalue_ref parse_resource_aquire_resp(const char *json);
static int find_source_port(jvalue_ref res);

static int dile_setup(int videoFormat, int width, int height, int redrawRate, void *context, int drFlags)
{
  int mrcCodec;
  switch (videoFormat)
  {
  case VIDEO_FORMAT_H264:
    video_fourcc = FOURCC_H264;
    mrcCodec = kVideoH264;
    gs_sps_init(width, height);
    printf("[DILE] Setup H264 encoding\n");
    break;
  case VIDEO_FORMAT_H265:
    video_fourcc = FOURCC_HEVC;
    mrcCodec = kVideoH265;
    printf("[DILE] Setup HEVC encoding\n");
    break;
  default:
    return -1;
  }
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

  jvalue_ref resreq = NULL;
  char *resresp = NULL;
  MRCResourceList resList = MRCCalcVdecResources(mrcCodec, width, height, redrawRate, kScanProgressive, k3DNone);
  resreq = serialize_resource_aquire_req(resList);
  ResourceManagerClientAcquire(rmhandle, jvalue_stringify(resreq), &resresp);
  MRCDeleteResourceList(resList);
  acquired_resources = parse_resource_aquire_resp(resresp);
  j_release(&resreq);
  free(resresp);

  if (dile_webos_version >= 5)
  {
    VideoOutputRegister(connId, appId);

    jvalue_ref reslist = jobject_get(acquired_resources, J_CSTR_TO_BUF("resources"));
    VideoOutputConnect(connId, find_source_port(reslist));
  }
  else
  {
    VideoSinkManagerRegister(connId);
  }

  if (dile_webos_version >= 5)
  {
    VideoOutputSetVideoData(connId, redrawRate, width, height);

    VideoOutputSetDisplayWindow(connId, true, 0, 0, width, height);
  }
  else
  {
    AcbSetMediaVideoData(connId, redrawRate, width, height);
  }

  ResourceManagerClientNotifyForeground(rmhandle);
  // VideoOutputBlankVideo(connId, true);

  if (DILE_VDEC_DIRECT_Open(video_fourcc, width, height, 0, 0) < 0)
  {
    fprintf(stderr, "Couldn't initialize video decoding %08x (%d)\n", video_fourcc, video_fourcc);
    return -1;
  }
  DILE_VDEC_DIRECT_SetCallback(playCallback);
  dile_buffer = malloc(DECODER_BUFFER_SIZE);
  memset(dile_buffer, 0, DECODER_BUFFER_SIZE);
  if (dile_buffer == NULL)
  {
    fprintf(stderr, "Not enough memory\n");
    DILE_VDEC_DIRECT_Close();
    return -1;
  }

  printf("[DILE] Video opened\n");
  return 0;
}

static void dile_cleanup()
{
  DILE_VDEC_DIRECT_Stop();

  DILE_VDEC_DIRECT_Close();

  if (rmhandle)
  {
    const char *connId = ResourceManagerClientGetConnectionID(rmhandle);
    const char *appId = getenv("APPID");

    if (dile_webos_version >= 5)
    {
      VideoOutputDisconnect(connId);

      VideoOutputUnregister(connId);
    }
    else
    {
      VideoSinkManagerUnregister(connId);
    }
    if (acquired_resources)
    {
      ResourceManagerClientRelease(rmhandle, jvalue_stringify(jobject_get(acquired_resources, J_CSTR_TO_BUF("resources"))));
      j_release(&acquired_resources);
      acquired_resources = NULL;
    }

    ResourceManagerClientUnregisterPipeline(rmhandle);

    ResourceManagerClientDestroy(rmhandle);
  }

  if (dile_buffer)
  {
    free(dile_buffer);
  }

  if (video_fourcc == FOURCC_H264)
  {
    gs_sps_finalize();
  }
}

static int dile_submit_decode_unit(PDECODE_UNIT decodeUnit)
{
  if (decodeUnit->fullLength < DECODER_BUFFER_SIZE)
  {
    int length = 0;
    for (PLENTRY entry = decodeUnit->bufferList; entry != NULL; entry = entry->next)
    {
      if (video_fourcc == FOURCC_H264 && entry->bufferType == BUFFER_TYPE_SPS)
      {
        gs_sps_fix(entry, GS_SPS_BITSTREAM_FIXUP, dile_buffer, &length);
      }
      else
      {
        memcpy(&dile_buffer[length], entry->data, entry->length);
        length += entry->length;
      }
    }
    if (video_fourcc == FOURCC_H264 && length % 4)
      length += (4 - (length % 4));
    if (DILE_VDEC_DIRECT_Play(dile_buffer, length) == -1)
    {
      fprintf(stderr, "DILE_VDEC_DIRECT_Play returned -1\n");
      return DR_OK;
    }
  }
  else
  {
    fprintf(stderr, "Video decode buffer too small, skip this frame");
    return DR_NEED_IDR;
  }

  return DR_OK;
}

jvalue_ref serialize_resource_aquire_req(MRCResourceList list)
{
  jvalue_ref root = jarray_create(0);
  for (int i = 0; list[i]; i++)
  {
    jarray_append(root, jobject_create_var(jkeyval(J_CSTR_TO_JVAL("resource"), j_cstr_to_jval(list[i]->type)),
                                           jkeyval(J_CSTR_TO_JVAL("qty"), jnumber_create_i32(list[i]->quantity))));
  }
  return root;
}

jvalue_ref parse_resource_aquire_resp(const char *json)
{

  JSchemaInfo schemaInfo;
  jschema_info_init(&schemaInfo, jschema_all(), NULL, NULL);
  jdomparser_ref parser = jdomparser_create(&schemaInfo, 0);
  jdomparser_feed(parser, json, strlen(json));

  jdomparser_end(parser);
  jvalue_ref result = jdomparser_get_result(parser);
  jdomparser_release(&parser);
  return result;
}

int find_source_port(jvalue_ref res)
{
  int result = 0;
  for (int i = 0; i < jarray_size(res); i++)
  {
    jvalue_ref item = jarray_get(res, i);
    int index = 0;
    if (jstring_equal2(jobject_get(item, J_CSTR_TO_BUF("resource")), J_CSTR_TO_BUF("VDEC")) &&
        jnumber_get_i32(jobject_get(item, J_CSTR_TO_BUF("index")), &index) == 0)
    {
      if (result == 0 || index < result)
        result = index;
    }
  }
  return result;
}

DECODER_RENDERER_CALLBACKS decoder_callbacks_dile = {
    .setup = dile_setup,
    .cleanup = dile_cleanup,
    .submitDecodeUnit = dile_submit_decode_unit,
    .capabilities = CAPABILITY_SLICES_PER_FRAME(4) | CAPABILITY_DIRECT_SUBMIT,
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

void playCallback(unsigned long long buffID)
{
  printf("playCallback: %lld\n", buffID);
}