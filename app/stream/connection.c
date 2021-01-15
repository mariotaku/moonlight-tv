/*
 * This file is part of Moonlight Embedded.
 *
 * Copyright (C) 2015-2017 Iwan Timmer
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

#include "src/connection.h"
#include "session.h"

#include <stdio.h>
#include <stdarg.h>
#include <signal.h>

bool connection_debug;
ConnListenerRumble rumble_handler = NULL;

static void connection_terminated(int errorCode)
{
  fprintf(stderr, "Connection terminated, errorCode = 0x%x\n", errorCode);
  _streaming_errmsg_write("Connection terminated, errorCode = 0x%x", errorCode);
  streaming_interrupt(false);
}

static void connection_log_message(const char *format, ...)
{
  va_list arglist;
  va_start(arglist, format);
  vprintf(format, arglist);
  va_end(arglist);
}

static void rumble(unsigned short controllerNumber, unsigned short lowFreqMotor, unsigned short highFreqMotor)
{
  if (rumble_handler)
    rumble_handler(controllerNumber, lowFreqMotor, highFreqMotor);
}

static void connection_status_update(int status)
{
  switch (status)
  {
  case CONN_STATUS_OKAY:
    printf("Connection is okay\n");
    break;
  case CONN_STATUS_POOR:
    printf("Connection is poor\n");
    break;
  }
}

static void connection_stage_failed(int stage, int errorCode)
{
  fprintf(stderr, "Connection failed at stage %d, errorCode = 0x%x\n", stage, errorCode);
  _streaming_errmsg_write("Connection failed at stage %d, errorCode = 0x%x", stage, errorCode);
}

CONNECTION_LISTENER_CALLBACKS connection_callbacks = {
    .stageStarting = NULL,
    .stageComplete = NULL,
    .stageFailed = connection_stage_failed,
    .connectionStarted = NULL,
    .connectionTerminated = connection_terminated,
    .logMessage = connection_log_message,
    .rumble = rumble,
    .connectionStatusUpdate = connection_status_update};
