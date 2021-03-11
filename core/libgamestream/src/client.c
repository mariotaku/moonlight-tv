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
#include <stdarg.h>
#include <uuid/uuid.h>
#include <mbedtls/sha1.h>
#include <mbedtls/sha256.h>
#include <mbedtls/aes.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/pk.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/rsa.h>
#include <mbedtls/error.h>

#include <assert.h>

#define UNIQUE_FILE_NAME "uniqueid.dat"

#define UNIQUEID_BYTES 8
#define UNIQUEID_CHARS (UNIQUEID_BYTES * 2)

static char unique_id[UNIQUEID_CHARS + 1];
static mbedtls_pk_context privateKey;
static mbedtls_x509_crt cert;
static char cert_bin[4096], cert_hex[4096];

const char *gs_error;

static bool generateSignature(const unsigned char *msg, size_t mlen, unsigned char *sig, size_t *slen, mbedtls_pk_context *pkey, mbedtls_ctr_drbg_context *rng);
static bool verifySignature(const unsigned char *msg, size_t mlen, const unsigned char *sig, size_t slen, mbedtls_x509_crt *cert);
static bool construct_url(char *url, size_t ulen, bool secure, const char *address, const char *action, const char *fmt, ...);
static int hash_data(mbedtls_md_type_t type, const unsigned char *input, size_t ilen, unsigned char *output, size_t *olen);
static bool crypt_data(mbedtls_aes_context *ctx, int mode, const unsigned char input[16], unsigned char output[16], size_t len);

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
    cert_bin[length / 2] = c;
    length += 2;
  }
  cert_bin[length / 2] = 0;
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

    // Modern GFE versions don't allow serverinfo to be fetched over HTTPS if the client
    // is not already paired. Since we can't pair without knowing the server version, we
    // make another request over HTTP if the HTTPS request fails. We can't just use HTTP
    // for everything because it doesn't accurately tell us if we're paired.
    construct_url(url, sizeof(url), i == 0, server->serverInfo.address, "serverinfo", NULL);

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

static void bytes_to_hex(const unsigned char *in, char *out, size_t len)
{
  for (int i = 0; i < len; i++)
  {
    sprintf(out + i * 2, "%02x", in[i]);
  }
  out[len * 2] = 0;
}

static void hex_to_bytes(const char *in, unsigned char *out, size_t *len)
{
  size_t inl = strlen(in);
  for (int count = 0; count < inl; count += 2)
  {
    sscanf(&in[count], "%2hhx", &out[count / 2]);
  }
  if (len)
  {
    *len = inl / 2;
  }
}

static bool generateSignature(const unsigned char *msg, size_t mlen, unsigned char *sig, size_t *slen, mbedtls_pk_context *pkey, mbedtls_ctr_drbg_context *rng)
{
  int result = 0;
  unsigned char hash[32];
  if ((result = mbedtls_sha256_ret(msg, mlen, hash, 0)) != 0)
  {
    goto cleanup;
  }

  if ((result = mbedtls_pk_sign(pkey, MBEDTLS_MD_SHA256, hash, 32, sig, slen, mbedtls_ctr_drbg_random, rng)) != 0)
  {
    goto cleanup;
  }

cleanup:
  return result == 0;
}

static bool verifySignature(const unsigned char *msg, size_t mlen, const unsigned char *sig, size_t slen, mbedtls_x509_crt *cert)
{
  int result = 0;
  unsigned char hash[32];
  if ((result = mbedtls_sha256_ret(msg, mlen, hash, 0)) != 0)
  {
    return false;
  }

  result = mbedtls_pk_verify(&cert->pk, MBEDTLS_MD_SHA256, hash, 32, sig, slen);

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
  construct_url(url, sizeof(url), false, server->serverInfo.address, "unpair", NULL);
  ret = http_request(url, data);

  http_free_data(data);
  return ret;
}

struct challenge_response_t
{
  unsigned char challenge[16];
  unsigned char signature[256];
  unsigned char secret[16];
};

int gs_pair(PSERVER_DATA server, char *pin)
{
  int svrversion = server->serverMajorVersion;
  mbedtls_md_type_t hash_algo = svrversion >= 7 ? MBEDTLS_MD_SHA256 : MBEDTLS_MD_SHA1;
  size_t hash_length;
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

  mbedtls_x509_crt server_cert;
  mbedtls_x509_crt_init(&server_cert);

  if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0)) != 0)
  {
    goto cleanup;
  }

  if (server->paired)
  {
    ret = GS_WRONG_STATE;
    gs_error = "Already paired";
    goto cleanup;
  }

  if (server->currentGame != 0)
  {
    ret = GS_WRONG_STATE;
    gs_error = "The computer is currently in a game. You must close the game before pairing";
    goto cleanup;
  }

  unsigned char salt[16];
  mbedtls_ctr_drbg_random(&ctr_drbg, salt, 16);
  char salt_hex[33];
  bytes_to_hex(salt, salt_hex, 16);

  unsigned char salted_pin[20];
  memcpy(salted_pin, salt, 16);
  memcpy(&salted_pin[16], pin, 4);

  unsigned char aes_key[32];

  hash_data(hash_algo, salted_pin, 20, aes_key, &hash_length);

  construct_url(url, sizeof(url), false, server->serverInfo.address, "pair",
                "devicename=roth&updateState=1&phrase=getservercert&salt=%s&clientcert=%s", salt_hex, cert_hex);
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

  char server_cert_str[8192];
  size_t plaincert_len = 0;
  hex_to_bytes(result, server_cert_str, &plaincert_len);
  server_cert_str[plaincert_len] = 0;

  if ((ret = mbedtls_x509_crt_parse(&server_cert, server_cert_str, plaincert_len + 1)) != 0)
  {
    char errstr[4096];
    mbedtls_strerror(ret, errstr, 4096);
    printf("%s\n", errstr);
    gs_error = "Failed to parse server cert";
    ret = GS_FAILED;
    goto cleanup;
  }

  mbedtls_aes_setkey_enc(&aes, aes_key, 128);
  mbedtls_aes_setkey_dec(&aes, aes_key, 128);

  unsigned char random_challenge[16];
  mbedtls_ctr_drbg_random(&ctr_drbg, random_challenge, 16);

  unsigned char encrypted_challenge[16];
  if (!crypt_data(&aes, MBEDTLS_AES_ENCRYPT, random_challenge, encrypted_challenge, 16))
  {
    gs_error = "Failed to encrypt random_challenge";
    ret = GS_FAILED;
    goto cleanup;
  }

  char encrypted_challenge_hex[33];
  bytes_to_hex(encrypted_challenge, encrypted_challenge_hex, 16);

  construct_url(url, sizeof(url), false, server->serverInfo.address, "pair",
                "devicename=roth&updateState=1&clientchallenge=%s", encrypted_challenge_hex);
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
  hex_to_bytes(result, challenge_response_data_enc, NULL);

  crypt_data(&aes, MBEDTLS_AES_DECRYPT, challenge_response_data_enc, challenge_response_data, 48);

  unsigned char client_secret_data[16];
  mbedtls_ctr_drbg_random(&ctr_drbg, client_secret_data, 16);

  struct challenge_response_t challenge_response;
  unsigned char challenge_response_hash[32], challenge_response_hash_enc[32];
  memset(challenge_response_hash, 0, sizeof(challenge_response_hash));
  memcpy(challenge_response.challenge, &challenge_response_data[hash_length], sizeof(challenge_response.challenge));
  memcpy(challenge_response.signature, cert.sig.p, sizeof(challenge_response.signature));
  memcpy(challenge_response.secret, client_secret_data, sizeof(challenge_response.secret));
  if (server->serverMajorVersion >= 7)
    mbedtls_sha256_ret((unsigned char *)&challenge_response, sizeof(struct challenge_response_t), challenge_response_hash, 0);
  else
    mbedtls_sha1_ret((unsigned char *)&challenge_response, sizeof(struct challenge_response_t), challenge_response_hash);

  crypt_data(&aes, MBEDTLS_AES_ENCRYPT, challenge_response_hash, challenge_response_hash_enc, 32);

  char challenge_response_hex[65];
  bytes_to_hex(challenge_response_hash_enc, challenge_response_hex, 32);

  construct_url(url, sizeof(url), false, server->serverInfo.address, "pair",
                "updateState=1&serverchallengeresp=%s", challenge_response_hex);
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

  struct
  {
    unsigned char secret[16];
    unsigned char signature[256];
  } pairing_secret;
  hex_to_bytes(result, (unsigned char *)&pairing_secret, NULL);

  if (!verifySignature(pairing_secret.secret, sizeof(pairing_secret.secret),
                       pairing_secret.signature, sizeof(pairing_secret.signature), &server_cert))
  {
    gs_error = "MITM attack detected";
    ret = GS_FAILED;
    goto cleanup;
  }

  struct challenge_response_t expected_response_data;
  memcpy(expected_response_data.challenge, random_challenge, 16);
  memcpy(expected_response_data.signature, server_cert.sig.p, 256);
  memcpy(expected_response_data.secret, pairing_secret.secret, 16);

  unsigned char expected_hash[32];
  hash_data(hash_algo, (unsigned char *)&expected_response_data, sizeof(expected_response_data),
            expected_hash, &hash_length);
  if (memcmp(expected_hash, challenge_response_data, hash_length) != 0)
  {
    gs_error = "Incorrect PIN";
    ret = GS_FAILED;
    goto cleanup;
  }

  unsigned char client_pairing_secret[16 + 256];
  memcpy(client_pairing_secret, client_secret_data, 16);
  size_t s_len;
  if (!generateSignature(client_pairing_secret, 16, &client_pairing_secret[16], &s_len, &privateKey, &ctr_drbg))
  {
    gs_error = "Failed to sign data";
    ret = GS_FAILED;
    goto cleanup;
  }

  char client_pairing_secret_hex[(16 + 256) * 2 + 1];
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
  mbedtls_entropy_free(&entropy);
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_x509_crt_free(&server_cert);

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

static bool construct_url(char *url, size_t ulen, bool secure, const char *address, const char *action, const char *fmt, ...)
{
  uuid_t uuid;
  char uuid_str[37];

  uuid_generate_random(uuid);
  uuid_unparse(uuid, uuid_str);

  if (fmt)
  {
    char params[4096];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(params, 4096, fmt, ap);
    va_end(ap);
    snprintf(url, ulen, "%s://%s:%d/%s?uniqueid=%s&uuid=%s&%s", secure ? "https" : "http", address, secure ? 47984 : 47989,
             action, unique_id, uuid_str, params);
  }
  else
  {
    snprintf(url, ulen, "%s://%s:%d/%s?uniqueid=%s&uuid=%s", secure ? "https" : "http", address, secure ? 47984 : 47989,
             action, unique_id, uuid_str);
  }
  return true;
}

int hash_data(mbedtls_md_type_t type, const unsigned char *input, size_t ilen, unsigned char *output, size_t *olen)
{
  switch (type)
  {
  case MBEDTLS_MD_SHA1:
    *olen = 20;
    return mbedtls_sha1_ret(input, ilen, output);
  case MBEDTLS_MD_SHA256:
    *olen = 32;
    return mbedtls_sha256_ret(input, ilen, output, 0);
  default:
    return -1;
  }
}

bool crypt_data(mbedtls_aes_context *ctx, int mode, const unsigned char *input, unsigned char *output, size_t len)
{
  for (int i = 0; i < len; i += 16)
  {
    if (mbedtls_aes_crypt_ecb(ctx, mode, &input[i], &output[i]) != 0)
    {
      return false;
    }
  }
  return true;
}