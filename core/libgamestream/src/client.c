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

#include "http.h"
#include "xml.h"
#include "mkcert.h"
#include "client.h"
#include "errors.h"
#include <linux/limits.h>
#include <errno.h>

#include <Limelight.h>

#include <sys/stat.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <uuid/uuid.h>
#include <mbedtls/sha1.h>
#include <mbedtls/sha256.h>
#include <mbedtls/aes.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/pk.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/rsa.h>

#define UNIQUE_FILE_NAME "uniqueid.dat"

#define UNIQUEID_BYTES 8
#define UNIQUEID_CHARS (UNIQUEID_BYTES * 2)

static char unique_id[UNIQUEID_CHARS + 1];
static mbedtls_pk_context privateKey;
static mbedtls_x509_crt cert;
static char cert_hex[4096];

const char *gs_error;

static int mkdirtree(const char *directory)
{
  char buffer[PATH_MAX];
  char *p = buffer;

  // The passed in string could be a string literal
  // so we must copy it first
  strncpy(p, directory, PATH_MAX - 1);
  buffer[PATH_MAX - 1] = '\0';

  while (*p != 0)
  {
    // Find the end of the path element
    do
    {
      p++;
    } while (*p != 0 && *p != '/');

    char oldChar = *p;
    *p = 0;

    // Create the directory if it doesn't exist already
    if (mkdir(buffer, 0775) == -1 && errno != EEXIST)
    {
      return -1;
    }

    *p = oldChar;
  }

  return 0;
}

static int load_unique_id(const char *keyDirectory)
{
  char uniqueFilePath[PATH_MAX];
  snprintf(uniqueFilePath, PATH_MAX, "%s/%s", keyDirectory, UNIQUE_FILE_NAME);

  FILE *fd = fopen(uniqueFilePath, "r");
  if (fd == NULL)
  {
    snprintf(unique_id, UNIQUEID_CHARS + 1, "0123456789ABCDEF");

    fd = fopen(uniqueFilePath, "w");
    if (fd == NULL)
      return GS_FAILED;

    fwrite(unique_id, UNIQUEID_CHARS, 1, fd);
  }
  else
  {
    fread(unique_id, UNIQUEID_CHARS, 1, fd);
  }
  fclose(fd);
  unique_id[UNIQUEID_CHARS] = 0;

  return GS_OK;
}

static int load_cert(const char *keyDirectory)
{
  char certificateFilePath[PATH_MAX];
  snprintf(certificateFilePath, PATH_MAX, "%s/%s", keyDirectory, CERTIFICATE_FILE_NAME);

  char keyFilePath[PATH_MAX];
  snprintf(&keyFilePath[0], PATH_MAX, "%s/%s", keyDirectory, KEY_FILE_NAME);

  FILE *fd = fopen(certificateFilePath, "r");
  if (fd == NULL)
  {
    printf("Generating certificate...");
    if (mkcert_generate(certificateFilePath, keyFilePath) == 0)
    {
      printf("done\n");
    }
    else
    {
      printf("\n");
      return GS_FAILED;
    }

    fd = fopen(certificateFilePath, "r");
  }

  if (fd == NULL || mbedtls_x509_crt_parse_file(&cert, certificateFilePath) != 0)
  {
    gs_error = "Can't open certificate file";
    return GS_FAILED;
  }

  int c;
  int length = 0;
  while ((c = fgetc(fd)) != EOF)
  {
    sprintf(&cert_hex[length], "%02x", c);
    length += 2;
  }
  cert_hex[length] = 0;

  fclose(fd);

  mbedtls_pk_init(&privateKey);
  if (mbedtls_pk_parse_keyfile(&privateKey, keyFilePath, NULL) != 0)
  {
    gs_error = "Error loading key into memory";
    return GS_FAILED;
  }

  return GS_OK;
}

static int load_server_status(PSERVER_DATA server)
{

  uuid_t uuid;
  char uuid_str[37];

  int ret;
  char url[4096];
  int i;

  i = 0;
  do
  {
    char *pairedText = NULL;
    char *currentGameText = NULL;
    char *stateText = NULL;
    char *serverCodecModeSupportText = NULL;

    ret = GS_INVALID;

    uuid_generate_random(uuid);
    uuid_unparse(uuid, uuid_str);

    // Modern GFE versions don't allow serverinfo to be fetched over HTTPS if the client
    // is not already paired. Since we can't pair without knowing the server version, we
    // make another request over HTTP if the HTTPS request fails. We can't just use HTTP
    // for everything because it doesn't accurately tell us if we're paired.
    snprintf(url, sizeof(url), "%s://%s:%d/serverinfo?uniqueid=%s&uuid=%s",
             i == 0 ? "https" : "http", server->serverInfo.address, i == 0 ? 47984 : 47989, unique_id, uuid_str);

    PHTTP_DATA data = http_create_data();
    if (data == NULL)
    {
      ret = GS_OUT_OF_MEMORY;
      goto cleanup;
    }
    if (http_request(url, data) != GS_OK)
    {
      ret = GS_IO_ERROR;
      goto cleanup;
    }

    if (xml_status(data->memory, data->size) == GS_ERROR)
    {
      ret = GS_ERROR;
      goto cleanup;
    }

    if (xml_search(data->memory, data->size, "uniqueid", &server->uuid) != GS_OK)
    {
      goto cleanup;
    }
    if (xml_search(data->memory, data->size, "mac", &server->mac) != GS_OK)
    {
      goto cleanup;
    }
    if (xml_search(data->memory, data->size, "hostname", &server->hostname) != GS_OK)
    {
      server->hostname = strdup(server->serverInfo.address);
    }

    if (xml_search(data->memory, data->size, "currentgame", &currentGameText) != GS_OK)
    {
      goto cleanup;
    }

    if (xml_search(data->memory, data->size, "PairStatus", &pairedText) != GS_OK)
      goto cleanup;

    if (xml_search(data->memory, data->size, "appversion", (char **)&server->serverInfo.serverInfoAppVersion) != GS_OK)
      goto cleanup;

    if (xml_search(data->memory, data->size, "state", &stateText) != GS_OK)
      goto cleanup;

    if (xml_search(data->memory, data->size, "ServerCodecModeSupport", &serverCodecModeSupportText) != GS_OK)
      goto cleanup;

    if (xml_search(data->memory, data->size, "gputype", &server->gpuType) != GS_OK)
      goto cleanup;

    if (xml_search(data->memory, data->size, "GsVersion", &server->gsVersion) != GS_OK)
      goto cleanup;

    if (xml_search(data->memory, data->size, "GfeVersion", (char **)&server->serverInfo.serverInfoGfeVersion) != GS_OK)
      goto cleanup;

    if (xml_modelist(data->memory, data->size, &server->modes) != GS_OK)
      goto cleanup;

    // These fields are present on all version of GFE that this client supports
    if (!strlen(currentGameText) || !strlen(pairedText) || !strlen(server->serverInfo.serverInfoAppVersion) || !strlen(stateText))
      goto cleanup;

    server->paired = pairedText != NULL && strcmp(pairedText, "1") == 0;
    server->currentGame = currentGameText == NULL ? 0 : atoi(currentGameText);
    server->supports4K = serverCodecModeSupportText != NULL;
    server->serverMajorVersion = atoi(server->serverInfo.serverInfoAppVersion);

    if (strstr(stateText, "_SERVER_BUSY") == NULL)
    {
      // After GFE 2.8, current game remains set even after streaming
      // has ended. We emulate the old behavior by forcing it to zero
      // if streaming is not active.
      server->currentGame = 0;
    }
    ret = GS_OK;

  cleanup:
    if (data != NULL)
      http_free_data(data);

    if (pairedText != NULL)
      free(pairedText);

    if (currentGameText != NULL)
      free(currentGameText);

    if (serverCodecModeSupportText != NULL)
      free(serverCodecModeSupportText);

    i++;
  } while (ret != GS_OK && i < 2);

  if (ret == GS_OK && !server->unsupported)
  {
    if (server->serverMajorVersion > MAX_SUPPORTED_GFE_VERSION)
    {
      gs_error = "Ensure you're running the latest version of Moonlight Embedded or downgrade GeForce Experience and try again";
      ret = GS_UNSUPPORTED_VERSION;
    }
    else if (server->serverMajorVersion < MIN_SUPPORTED_GFE_VERSION)
    {
      gs_error = "Moonlight Embedded requires a newer version of GeForce Experience. Please upgrade GFE on your PC and try again.";
      ret = GS_UNSUPPORTED_VERSION;
    }
  }

  return ret;
}

static void bytes_to_hex(unsigned char *in, char *out, size_t len)
{
  for (int i = 0; i < len; i++)
  {
    sprintf(out + i * 2, "%02x", in[i]);
  }
  out[len * 2] = 0;
}

static bool sign_it(const unsigned char *msg, size_t mlen, unsigned char *sig, size_t *slen, mbedtls_pk_context *pkey, mbedtls_ctr_drbg_context *rng)
{
  int result = 0;

  *slen = 0;

  unsigned char hash[32];
  if ((result = mbedtls_sha256_ret(msg, mlen, hash, 0)) != 0)
  {
    goto cleanup;
  }

  mbedtls_rsa_context *rsa = mbedtls_pk_rsa(*pkey);
  if ((result = mbedtls_rsa_pkcs1_sign(rsa, mbedtls_ctr_drbg_random, rng, MBEDTLS_RSA_PRIVATE, MBEDTLS_MD_SHA256, 32, hash, sig)) != 0)
  {
    goto cleanup;
  }

cleanup:
  return result == 0;
}

static bool verifySignature(const unsigned char *data, int dataLength, unsigned char *signature, int signatureLength, const char *cert)
{
  int result = 0;
  mbedtls_x509_crt x509;
  mbedtls_x509_crt_init(&x509);

  if ((result = mbedtls_x509_crt_parse(&x509, cert, strlen(cert) + 1)) != 0)
  {
    goto cleanup;
  }

  unsigned char hash[32];
  if ((result = mbedtls_sha256_ret(data, dataLength, hash, 0)) != 0)
  {
    goto cleanup;
  }

  result = mbedtls_pk_verify(&x509.pk, MBEDTLS_MD_SHA256, hash, 32, signature, signatureLength);

cleanup:
  mbedtls_x509_crt_free(&x509);
  return result == 0;
}

int gs_unpair(PSERVER_DATA server)
{
  int ret = GS_OK;
  char url[4096];
  uuid_t uuid;
  char uuid_str[37];
  PHTTP_DATA data = http_create_data();
  if (data == NULL)
    return GS_OUT_OF_MEMORY;

  uuid_generate_random(uuid);
  uuid_unparse(uuid, uuid_str);
  snprintf(url, sizeof(url), "http://%s:47989/unpair?uniqueid=%s&uuid=%s", server->serverInfo.address, unique_id, uuid_str);
  ret = http_request(url, data);

  http_free_data(data);
  return ret;
}

int gs_pair(PSERVER_DATA server, char *pin)
{
  int ret = GS_OK;
  char *result = NULL;
  char url[4096];
  uuid_t uuid;
  char uuid_str[37];

  mbedtls_entropy_context entropy;
  mbedtls_entropy_init(&entropy);

  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_ctr_drbg_init(&ctr_drbg);

  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);

  if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0)) != 0)
  {
    goto cleanup;
  }

  if (server->paired)
  {
    gs_error = "Already paired";
    return GS_WRONG_STATE;
  }

  if (server->currentGame != 0)
  {
    gs_error = "The computer is currently in a game. You must close the game before pairing";
    return GS_WRONG_STATE;
  }

  unsigned char salt_data[16];
  char salt_hex[33];
  mbedtls_ctr_drbg_random(&ctr_drbg, salt_data, 16);
  bytes_to_hex(salt_data, salt_hex, 16);

  uuid_generate_random(uuid);
  uuid_unparse(uuid, uuid_str);
  snprintf(url, sizeof(url), "http://%s:47989/pair?uniqueid=%s&uuid=%s&devicename=roth&updateState=1&phrase=getservercert&salt=%s&clientcert=%s", server->serverInfo.address, unique_id, uuid_str, salt_hex, cert_hex);
  PHTTP_DATA data = http_create_data();
  if (data == NULL)
    return GS_OUT_OF_MEMORY;
  else if ((ret = http_request(url, data)) != GS_OK)
    goto cleanup;

  if ((ret = xml_status(data->memory, data->size) != GS_OK))
    goto cleanup;
  else if ((ret = xml_search(data->memory, data->size, "paired", &result)) != GS_OK)
    goto cleanup;

  if (strcmp(result, "1") != 0)
  {
    gs_error = "Failed pairing at stage #1";
    ret = GS_FAILED;
    goto cleanup;
  }

  free(result);
  result = NULL;
  if ((ret = xml_search(data->memory, data->size, "plaincert", &result)) != GS_OK)
  {
    gs_error = "Failed to parse plaincert";
    ret = GS_FAILED;
    goto cleanup;
  }

  if (strlen(result) / 2 > 8191)
  {
    gs_error = "Server certificate too big";
    ret = GS_FAILED;
    goto cleanup;
  }

  char plaincert[8192];
  for (int count = 0; count < strlen(result); count += 2)
  {
    sscanf(&result[count], "%2hhx", &plaincert[count / 2]);
  }
  plaincert[strlen(result) / 2] = '\0';

  unsigned char salt_pin[20];
  unsigned char aes_key_hash[32];
  memcpy(salt_pin, salt_data, 16);
  memcpy(salt_pin + 16, pin, 4);

  int hash_length = server->serverMajorVersion >= 7 ? 32 : 20;
  if (server->serverMajorVersion >= 7)
    mbedtls_sha256_ret(salt_pin, 20, aes_key_hash, 0);
  else
    mbedtls_sha1_ret(salt_pin, 20, aes_key_hash);

  mbedtls_aes_setkey_enc(&aes, aes_key_hash, 128);
  mbedtls_aes_setkey_dec(&aes, aes_key_hash, 128);

  unsigned char challenge_data[16];
  unsigned char challenge_enc[16];
  char challenge_hex[33];
  mbedtls_ctr_drbg_random(&ctr_drbg, challenge_data, 16);
  mbedtls_aes_encrypt(&aes, challenge_data, challenge_enc);
  bytes_to_hex(challenge_enc, challenge_hex, 16);

  uuid_generate_random(uuid);
  uuid_unparse(uuid, uuid_str);
  snprintf(url, sizeof(url), "http://%s:47989/pair?uniqueid=%s&uuid=%s&devicename=roth&updateState=1&clientchallenge=%s", server->serverInfo.address, unique_id, uuid_str, challenge_hex);
  if ((ret = http_request(url, data)) != GS_OK)
    goto cleanup;

  free(result);
  result = NULL;
  if ((ret = xml_status(data->memory, data->size) != GS_OK))
    goto cleanup;
  else if ((ret = xml_search(data->memory, data->size, "paired", &result)) != GS_OK)
    goto cleanup;

  if (strcmp(result, "1") != 0)
  {
    gs_error = "Failed pairing at stage #2";
    ret = GS_FAILED;
    goto cleanup;
  }

  free(result);
  result = NULL;
  if (xml_search(data->memory, data->size, "challengeresponse", &result) != GS_OK)
  {
    ret = GS_INVALID;
    goto cleanup;
  }

  unsigned char challenge_response_data_enc[48];
  unsigned char challenge_response_data[48];
  for (int count = 0; count < strlen(result); count += 2)
  {
    sscanf(&result[count], "%2hhx", &challenge_response_data_enc[count / 2]);
  }

  for (int i = 0; i < 48; i += 16)
  {
    mbedtls_aes_decrypt(&aes, &challenge_response_data_enc[i], &challenge_response_data[i]);
  }

  unsigned char client_secret_data[16];
  mbedtls_ctr_drbg_random(&ctr_drbg, client_secret_data, 16);

  unsigned char challenge_response[16 + 256 + 16];
  unsigned char challenge_response_hash[32];
  unsigned char challenge_response_hash_enc[32];
  char challenge_response_hex[65];
  memcpy(challenge_response, challenge_response_data + hash_length, 16);
  memcpy(challenge_response + 16, cert.sig.p, 256);
  memcpy(challenge_response + 16 + 256, client_secret_data, 16);
  if (server->serverMajorVersion >= 7)
    mbedtls_sha256_ret(challenge_response, 16 + 256 + 16, challenge_response_hash, 0);
  else
    mbedtls_sha1_ret(challenge_response, 16 + 256 + 16, challenge_response_hash);

  for (int i = 0; i < 32; i += 16)
  {
    mbedtls_aes_encrypt(&aes, &challenge_response_hash[i], &challenge_response_hash_enc[i]);
  }
  bytes_to_hex(challenge_response_hash_enc, challenge_response_hex, 32);

  uuid_generate_random(uuid);
  uuid_unparse(uuid, uuid_str);
  snprintf(url, sizeof(url), "http://%s:47989/pair?uniqueid=%s&uuid=%s&devicename=roth&updateState=1&serverchallengeresp=%s", server->serverInfo.address, unique_id, uuid_str, challenge_response_hex);
  if ((ret = http_request(url, data)) != GS_OK)
    goto cleanup;

  free(result);
  result = NULL;
  if ((ret = xml_status(data->memory, data->size) != GS_OK))
    goto cleanup;
  else if ((ret = xml_search(data->memory, data->size, "paired", &result)) != GS_OK)
    goto cleanup;

  if (strcmp(result, "1") != 0)
  {
    gs_error = "Failed pairing at stage #3";
    ret = GS_FAILED;
    goto cleanup;
  }

  free(result);
  result = NULL;
  if (xml_search(data->memory, data->size, "pairingsecret", &result) != GS_OK)
  {
    ret = GS_INVALID;
    goto cleanup;
  }

  unsigned char pairing_secret[16 + 256];
  for (int count = 0; count < strlen(result); count += 2)
  {
    sscanf(&result[count], "%2hhx", &pairing_secret[count / 2]);
  }

  if (!verifySignature(pairing_secret, 16, pairing_secret + 16, 256, plaincert))
  {
    gs_error = "MITM attack detected";
    ret = GS_FAILED;
    goto cleanup;
  }

  unsigned char signature[256];
  size_t s_len;
  if (!sign_it(client_secret_data, 16, signature, &s_len, &privateKey, &ctr_drbg))
  {
    gs_error = "Failed to sign data";
    ret = GS_FAILED;
    goto cleanup;
  }

  unsigned char client_pairing_secret[16 + 256];
  char client_pairing_secret_hex[(16 + 256) * 2 + 1];
  memcpy(client_pairing_secret, client_secret_data, 16);
  memcpy(&client_pairing_secret[16], signature, 256);
  bytes_to_hex(client_pairing_secret, client_pairing_secret_hex, 16 + 256);

  uuid_generate_random(uuid);
  uuid_unparse(uuid, uuid_str);
  snprintf(url, sizeof(url), "http://%s:47989/pair?uniqueid=%s&uuid=%s&devicename=roth&updateState=1&clientpairingsecret=%s", server->serverInfo.address, unique_id, uuid_str, client_pairing_secret_hex);
  if ((ret = http_request(url, data)) != GS_OK)
    goto cleanup;

  free(result);
  result = NULL;
  if ((ret = xml_status(data->memory, data->size) != GS_OK))
    goto cleanup;
  else if ((ret = xml_search(data->memory, data->size, "paired", &result)) != GS_OK)
    goto cleanup;

  if (strcmp(result, "1") != 0)
  {
    gs_error = "Failed pairing at stage #4";
    ret = GS_FAILED;
    goto cleanup;
  }

  uuid_generate_random(uuid);
  uuid_unparse(uuid, uuid_str);
  snprintf(url, sizeof(url), "https://%s:47984/pair?uniqueid=%s&uuid=%s&devicename=roth&updateState=1&phrase=pairchallenge", server->serverInfo.address, unique_id, uuid_str);
  if ((ret = http_request(url, data)) != GS_OK)
    goto cleanup;

  free(result);
  result = NULL;
  if ((ret = xml_status(data->memory, data->size) != GS_OK))
    goto cleanup;
  else if ((ret = xml_search(data->memory, data->size, "paired", &result)) != GS_OK)
    goto cleanup;

  if (strcmp(result, "1") != 0)
  {
    gs_error = "Failed pairing at stage #5";
    ret = GS_FAILED;
    goto cleanup;
  }

  server->paired = true;

cleanup:
  if (ret != GS_OK)
    gs_unpair(server);

  if (result != NULL)
    free(result);

  http_free_data(data);

  mbedtls_aes_free(&aes);
  mbedtls_ctr_drbg_free(&ctr_drbg);

  return ret;
}

int gs_applist(PSERVER_DATA server, PAPP_LIST *list)
{
  int ret = GS_OK;
  char url[4096];
  uuid_t uuid;
  char uuid_str[37];
  PHTTP_DATA data = http_create_data();
  if (data == NULL)
    return GS_OUT_OF_MEMORY;

  uuid_generate_random(uuid);
  uuid_unparse(uuid, uuid_str);
  snprintf(url, sizeof(url), "https://%s:47984/applist?uniqueid=%s&uuid=%s", server->serverInfo.address, unique_id, uuid_str);
  if (http_request(url, data) != GS_OK)
    ret = GS_IO_ERROR;
  else if (xml_status(data->memory, data->size) == GS_ERROR)
    ret = GS_ERROR;
  else if (xml_applist(data->memory, data->size, list) != GS_OK)
    ret = GS_INVALID;

  http_free_data(data);
  return ret;
}

int gs_start_app(PSERVER_DATA server, STREAM_CONFIGURATION *config, int appId, bool sops, bool localaudio, int gamepad_mask)
{
  int ret = GS_OK;
  uuid_t uuid;
  char *result = NULL;
  char uuid_str[37];

  mbedtls_entropy_context entropy;
  mbedtls_entropy_init(&entropy);

  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_ctr_drbg_init(&ctr_drbg);

  if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0)) != 0)
  {
    goto cleanup;
  }

  PDISPLAY_MODE mode = server->modes;
  bool correct_mode = false;
  bool supported_resolution = false;
  while (mode != NULL)
  {
    if (mode->width == config->width && mode->height == config->height)
    {
      supported_resolution = true;
      if (mode->refresh == config->fps)
        correct_mode = true;
    }

    mode = mode->next;
  }

  if (!correct_mode && !server->unsupported)
    return GS_NOT_SUPPORTED_MODE;
  else if (sops && !supported_resolution)
    return GS_NOT_SUPPORTED_SOPS_RESOLUTION;

  if (config->height >= 2160 && !server->supports4K)
    return GS_NOT_SUPPORTED_4K;

  mbedtls_ctr_drbg_random(&ctr_drbg, (unsigned char *)config->remoteInputAesKey, 16);
  memset(config->remoteInputAesIv, 0, 16);

  srand(time(NULL));
  char url[4096];
  u_int32_t rikeyid = 0;
  char rikey_hex[33];
  bytes_to_hex((unsigned char *)config->remoteInputAesKey, rikey_hex, 16);

  PHTTP_DATA data = http_create_data();
  if (data == NULL)
    return GS_OUT_OF_MEMORY;

  uuid_generate_random(uuid);
  uuid_unparse(uuid, uuid_str);
  int surround_info = SURROUNDAUDIOINFO_FROM_AUDIO_CONFIGURATION(config->audioConfiguration);
  if (server->currentGame == 0)
  {
    // Using an FPS value over 60 causes SOPS to default to 720p60,
    // so force it to 0 to ensure the correct resolution is set. We
    // used to use 60 here but that locked the frame rate to 60 FPS
    // on GFE 3.20.3.
    int fps = config->fps > 60 ? 0 : config->fps;
    snprintf(url, sizeof(url), "https://%s:47984/launch?uniqueid=%s&uuid=%s&appid=%d&mode=%dx%dx%d&additionalStates=1&sops=%d&rikey=%s&rikeyid=%d&localAudioPlayMode=%d&surroundAudioInfo=%d&remoteControllersBitmap=%d&gcmap=%d", server->serverInfo.address, unique_id, uuid_str, appId, config->width, config->height, fps, sops, rikey_hex, rikeyid, localaudio, surround_info, gamepad_mask, gamepad_mask);
  }
  else
    snprintf(url, sizeof(url), "https://%s:47984/resume?uniqueid=%s&uuid=%s&rikey=%s&rikeyid=%d&surroundAudioInfo=%d", server->serverInfo.address, unique_id, uuid_str, rikey_hex, rikeyid, surround_info);

  if ((ret = http_request(url, data)) == GS_OK)
    server->currentGame = appId;
  else
    goto cleanup;

  if ((ret = xml_status(data->memory, data->size) != GS_OK))
    goto cleanup;
  else if ((ret = xml_search(data->memory, data->size, "gamesession", &result)) != GS_OK)
    goto cleanup;

  if (!strcmp(result, "0"))
  {
    ret = GS_FAILED;
    goto cleanup;
  }

cleanup:
  if (result != NULL)
    free(result);

  http_free_data(data);

  mbedtls_ctr_drbg_free(&ctr_drbg);
  return ret;
}

int gs_quit_app(PSERVER_DATA server)
{
  int ret = GS_OK;
  char url[4096];
  uuid_t uuid;
  char uuid_str[37];
  char *result = NULL;
  PHTTP_DATA data = http_create_data();
  if (data == NULL)
    return GS_OUT_OF_MEMORY;

  uuid_generate_random(uuid);
  uuid_unparse(uuid, uuid_str);
  snprintf(url, sizeof(url), "https://%s:47984/cancel?uniqueid=%s&uuid=%s", server->serverInfo.address, unique_id, uuid_str);
  if ((ret = http_request(url, data)) != GS_OK)
    goto cleanup;

  if ((ret = xml_status(data->memory, data->size) != GS_OK))
    goto cleanup;
  else if ((ret = xml_search(data->memory, data->size, "cancel", &result)) != GS_OK)
    goto cleanup;

  if (strcmp(result, "0") == 0)
  {
    ret = GS_FAILED;
    goto cleanup;
  }

cleanup:
  if (result != NULL)
    free(result);

  http_free_data(data);
  return ret;
}

int gs_download_cover(PSERVER_DATA server, int appid, const char *path)
{
  int ret = GS_OK;
  char url[4096];
  uuid_t uuid;
  char uuid_str[37];
  PHTTP_DATA data = http_create_data();
  if (data == NULL)
    return GS_OUT_OF_MEMORY;

  uuid_generate_random(uuid);
  uuid_unparse(uuid, uuid_str);
  snprintf(url, sizeof(url), "https://%s:47984/appasset?uniqueid=%s&uuid=%s&appid=%d&AssetType=2&AssetIdx=0",
           server->serverInfo.address, unique_id, uuid_str, appid);
  ret = http_request(url, data);
  if (ret != GS_OK)
    goto cleanup;

  FILE *f = fopen(path, "wb");
  if (!f)
  {
    ret = GS_IO_ERROR;
    goto cleanup;
  }

  fwrite(data->memory, data->size, 1, f);
  fflush(f);
  fclose(f);

cleanup:
  http_free_data(data);
  return ret;
}

int gs_init(PSERVER_DATA server, char *address, const char *keyDirectory, int log_level, bool unsupported)
{
  mkdirtree(keyDirectory);
  if (load_unique_id(keyDirectory) != GS_OK)
    return GS_FAILED;

  if (load_cert(keyDirectory))
    return GS_FAILED;

  http_init(keyDirectory, log_level);

  LiInitializeServerInformation(&server->serverInfo);
  server->serverInfo.address = address;
  server->unsupported = unsupported;
  return load_server_status(server);
}
