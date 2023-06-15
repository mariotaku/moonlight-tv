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
#include <errno.h>

#include <Limelight.h>

#include <sys/stat.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <mbedtls/sha1.h>
#include <mbedtls/sha256.h>
#include <mbedtls/aes.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/pk.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/rsa.h>
#include <mbedtls/error.h>

#if __WIN32

#include <windows.h>

#define PATH_SEPARATOR '\\'
#else

#include <uuid/uuid.h>
#include <arpa/inet.h>

#if __linux

#include <linux/limits.h>

#endif

#define PATH_SEPARATOR '/'
#endif

#include <assert.h>

#include "crypto.h"

#define UNIQUE_FILE_NAME "uniqueid.dat"

#define UNIQUEID_BYTES 8
#define UNIQUEID_CHARS (UNIQUEID_BYTES * 2)

struct GS_CLIENT_T {
    char unique_id[UNIQUEID_CHARS + 1];
    mbedtls_pk_context pk;
    mbedtls_x509_crt cert;
    char cert_hex[8192];
    HTTP http;
};

const char *gs_error;

static char mbed_err[1024];

static bool construct_url(GS_CLIENT, char *url, size_t ulen, bool secure, const char *address, const char *action,
                          const char *fmt, ...);

#if __WIN32
#define MKDIR(path) mkdir(path)
#else
#define MKDIR(path) mkdir(path, 0775)
#endif

static int mkdirtree(const char *directory) {
    char buffer[PATH_MAX];
    char *p = buffer;

    // The passed in string could be a string literal
    // so we must copy it first
    strncpy(p, directory, PATH_MAX - 1);
    buffer[PATH_MAX - 1] = '\0';

    while (*p != 0) {
        // Find the end of the path element
        do {
            p++;
        } while (*p != 0 && *p != '/');

        char oldChar = *p;
        *p = 0;

        // Create the directory if it doesn't exist already
        if (MKDIR(buffer) == -1 && errno != EEXIST) {
            return -1;
        }

        *p = oldChar;
    }

    return 0;
}

static int load_unique_id(struct GS_CLIENT_T *hnd, const char *keyDirectory) {
    char uniqueFilePath[PATH_MAX];
    snprintf(uniqueFilePath, PATH_MAX, "%s%c%s", keyDirectory, PATH_SEPARATOR, UNIQUE_FILE_NAME);

    FILE *fd = fopen(uniqueFilePath, "r");
    if (fd == NULL || fread(hnd->unique_id, UNIQUEID_CHARS, 1, fd) != UNIQUEID_CHARS) {
        snprintf(hnd->unique_id, UNIQUEID_CHARS + 1, "0123456789ABCDEF");
        if (fd) {
            fclose(fd);
        }
        fd = fopen(uniqueFilePath, "w");
        if (fd == NULL) {
            static char msg[1024];
            snprintf(msg, 1023, "Failed to open unique ID file %s", uniqueFilePath);
            gs_error = msg;
            return GS_FAILED;
        }

        fwrite(hnd->unique_id, UNIQUEID_CHARS, 1, fd);
    }
    fclose(fd);
    hnd->unique_id[UNIQUEID_CHARS] = 0;

    return GS_OK;
}

static int load_cert(struct GS_CLIENT_T *hnd, const char *keyDirectory) {
    char certificateFilePath[PATH_MAX];
    snprintf(certificateFilePath, PATH_MAX, "%s%c%s", keyDirectory, PATH_SEPARATOR, CERTIFICATE_FILE_NAME);

    char keyFilePath[PATH_MAX];
    snprintf(&keyFilePath[0], PATH_MAX, "%s%c%s", keyDirectory, PATH_SEPARATOR, KEY_FILE_NAME);

    int mbedret;
    mbedtls_x509_crt_init(&hnd->cert);
    if ((mbedret = mbedtls_x509_crt_parse_file(&hnd->cert, certificateFilePath)) != 0) {
        printf("Generating certificate...");
        if (mkcert_generate(certificateFilePath, keyFilePath) != 0) {
            printf("failed.\n");
            gs_error = "Failed to generate certificate.";
            mbedtls_x509_crt_free(&hnd->cert);
            return GS_FAILED;
        }
        if ((mbedret = mbedtls_x509_crt_parse_file(&hnd->cert, certificateFilePath)) != 0) {
            mbedtls_strerror(mbedret, mbed_err, sizeof(mbed_err));
            gs_error = mbed_err;
            mbedtls_x509_crt_free(&hnd->cert);
            return GS_FAILED;
        }
    }
    FILE *fd = fopen(certificateFilePath, "r");
    assert(fd);
    int c;
    int length = 0;
    while ((c = fgetc(fd)) != EOF) {
        sprintf(&hnd->cert_hex[length], "%02x", c);
        length += 2;
    }
    hnd->cert_hex[length] = 0;

    fclose(fd);

    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ctr_drbg_init(&ctr_drbg);

    mbedtls_pk_init(&hnd->pk);
#if MBEDTLS_VERSION_NUMBER >= 0x03020100
    if (mbedtls_pk_parse_keyfile(&hnd->pk, keyFilePath, NULL, mbedtls_ctr_drbg_random, &ctr_drbg) != 0)
#else
    if (mbedtls_pk_parse_keyfile(&hnd->pk, keyFilePath, NULL) != 0)
#endif
    {
        mbedtls_pk_free(&hnd->pk);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        gs_error = "Error loading key into memory";
        return GS_FAILED;
    }
    mbedtls_ctr_drbg_free(&ctr_drbg);

    return GS_OK;
}

static int load_server_status(GS_CLIENT hnd, PSERVER_DATA server) {
    int ret;
    char url[4096];
    int i;

    i = 0;
    do {
        char *pairedText = NULL;
        char *currentGameText = NULL;
        char *stateText = NULL;
        char *serverCodecModeSupportText = NULL;

        ret = GS_INVALID;

        // Modern GFE versions don't allow serverinfo to be fetched over HTTPS if the client
        // is not already paired. Since we can't pair without knowing the server version, we
        // make another request over HTTP if the HTTPS request fails. We can't just use HTTP
        // for everything because it doesn't accurately tell us if we're paired.
        construct_url(hnd, url, sizeof(url), i == 0, server->serverInfo.address, "serverinfo", NULL);

        PHTTP_DATA data = http_create_data();
        if (data == NULL) {
            ret = GS_OUT_OF_MEMORY;
            goto cleanup;
        }
        if (http_request(hnd->http, url, data) != GS_OK) {
            ret = GS_IO_ERROR;
            goto cleanup;
        }

        if (xml_status(data->memory, data->size) == GS_ERROR) {
            ret = GS_ERROR;
            goto cleanup;
        }

        if (xml_search(data->memory, data->size, "uniqueid", (char **) &server->uuid) != GS_OK) {
            goto cleanup;
        }
        if (xml_search(data->memory, data->size, "mac", (char **) &server->mac) != GS_OK) {
            goto cleanup;
        }
        if (xml_search(data->memory, data->size, "hostname", (char **) &server->hostname) != GS_OK) {
            server->hostname = strdup(server->serverInfo.address);
        }

        if (xml_search(data->memory, data->size, "currentgame", &currentGameText) != GS_OK) {
            goto cleanup;
        }

        if (xml_search(data->memory, data->size, "PairStatus", &pairedText) != GS_OK)
            goto cleanup;

        if (xml_search(data->memory, data->size, "appversion", (char **) &server->serverInfo.serverInfoAppVersion) !=
            GS_OK)
            goto cleanup;

        if (xml_search(data->memory, data->size, "state", &stateText) != GS_OK)
            goto cleanup;

        if (xml_search(data->memory, data->size, "ServerCodecModeSupport", &serverCodecModeSupportText) != GS_OK)
            goto cleanup;

        if (xml_search(data->memory, data->size, "gputype", (char **) &server->gpuType) != GS_OK)
            goto cleanup;

        if (xml_search(data->memory, data->size, "GsVersion", (char **) &server->gsVersion) != GS_OK)
            goto cleanup;

        if (xml_search(data->memory, data->size, "GfeVersion", (char **) &server->serverInfo.serverInfoGfeVersion) !=
            GS_OK)
            goto cleanup;

        if (xml_modelist(data->memory, data->size, &server->modes) != GS_OK)
            goto cleanup;

        // These fields are present on all version of GFE that this client supports
        if (!strlen(currentGameText) || !strlen(pairedText) || !strlen(server->serverInfo.serverInfoAppVersion) ||
            !strlen(stateText))
            goto cleanup;

        int serverCodecModeSupport = serverCodecModeSupportText == NULL ? 0 : atoi(serverCodecModeSupportText);

        server->paired = pairedText != NULL && strcmp(pairedText, "1") == 0;
        server->currentGame = currentGameText == NULL ? 0 : atoi(currentGameText);
        server->supports4K = serverCodecModeSupport != 0;
        server->supportsHdr = serverCodecModeSupport & 0x200;
        server->serverMajorVersion = atoi(server->serverInfo.serverInfoAppVersion);

        if (strstr(stateText, "_SERVER_BUSY") == NULL) {
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

        if (stateText != NULL)
            free(stateText);

        if (serverCodecModeSupportText != NULL)
            free(serverCodecModeSupportText);

        i++;
    } while (ret != GS_OK && i < 2);

    if (ret == GS_OK && !server->unsupported) {
        if (server->serverMajorVersion > MAX_SUPPORTED_GFE_VERSION) {
            gs_error = "Ensure you're running the latest version of Moonlight Embedded or downgrade GeForce Experience and try again";
            ret = GS_UNSUPPORTED_VERSION;
        } else if (server->serverMajorVersion < MIN_SUPPORTED_GFE_VERSION) {
            gs_error = "Moonlight Embedded requires a newer version of GeForce Experience. Please upgrade GFE on your PC and try again.";
            ret = GS_UNSUPPORTED_VERSION;
        }
    }

    return ret;
}

static void bytes_to_hex(const unsigned char *in, char *out, size_t len) {
    for (int i = 0; i < len; i++) {
        sprintf(out + i * 2, "%02x", in[i]);
    }
    out[len * 2] = 0;
}

static void hex_to_bytes(const char *in, unsigned char *out, size_t maxlen, size_t *len) {
    static const uint8_t map_table[] = {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, /* 0x30(0)-0x3F(?) */
            0, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x40(@)-0x4F(O) */
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x50(P)-0x4F(_) */
            0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, /* 0x60(`)-0x66(f) */
    };
    size_t inl = strlen(in);
    if (inl % 2) return;
    assert (maxlen >= inl / 2);
    for (int count = 0; count < inl; count += 2) {
        char ch1 = in[count], ch2 = in[count + 1];
        if (ch1 < 0x30 || ch1 > 0x66 || ch2 < 0x30 || ch2 > 0x66) return;
        out[count / 2] = (map_table[ch1 - 0x30] << 4 | map_table[ch2 - 0x30]) & 0xFF;
    }
    if (len) {
        *len = inl / 2;
    }
}

int gs_unpair(GS_CLIENT hnd, PSERVER_DATA server) {
    int ret = GS_OK;
    char url[4096];
    PHTTP_DATA data = http_create_data();
    if (data == NULL)
        return GS_OUT_OF_MEMORY;

    construct_url(hnd, url, sizeof(url), false, server->serverInfo.address, "unpair", NULL);
    ret = http_request(hnd->http, url, data);
    if (ret != GS_OK)
        goto cleanup;

    if (xml_status(data->memory, data->size) == GS_ERROR) {
        ret = GS_ERROR;
        goto cleanup;
    }

    ret = GS_OK;
    server->paired = false;

    cleanup:
    http_free_data(data);

    return ret;
}

struct challenge_response_t {
    unsigned char challenge[16];
    unsigned char signature[256];
    unsigned char secret[16];
};

struct pairing_secret_t {
    unsigned char secret[16];
    unsigned char signature[256];
};

int gs_pair(GS_CLIENT hnd, PSERVER_DATA server, const char *pin) {
    mbedtls_md_type_t hash_algo;
    size_t hash_length;
    if (server->serverMajorVersion >= 7) {
        // Gen 7+ uses SHA-256 hashing
        hash_algo = MBEDTLS_MD_SHA256;
        hash_length = 32;
    } else {
        // Prior to Gen 7, SHA-1 is used
        hash_algo = MBEDTLS_MD_SHA1;
        hash_length = 20;
    }
    int ret = GS_OK;
    char *result = NULL;
    char url[5120];
    PHTTP_DATA data = NULL;

    mbedtls_entropy_context entropy;
    mbedtls_entropy_init(&entropy);

    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ctr_drbg_init(&ctr_drbg);

    mbedtls_aes_context aes_enc, aes_dec;
    mbedtls_aes_init(&aes_enc);
    mbedtls_aes_init(&aes_dec);

    mbedtls_x509_crt server_cert;
    mbedtls_x509_crt_init(&server_cert);

    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0)) != 0) {
        goto cleanup;
    }

    if (server->paired) {
        ret = GS_WRONG_STATE;
        gs_error = "Already paired";
        goto cleanup;
    }

    if (server->currentGame != 0) {
        ret = GS_WRONG_STATE;
        gs_error = "The computer is currently in a game. You must close the game before pairing";
        goto cleanup;
    }

    // Generate the salted pin, then create an AES key from them
    struct {
        unsigned char salt[16];
        unsigned char pin[4];
    } salted_pin;
    memcpy(salted_pin.pin, pin, 4);
    mbedtls_ctr_drbg_random(&ctr_drbg, salted_pin.salt, 16);

    char salt_hex[33];
    bytes_to_hex(salted_pin.salt, salt_hex, 16);

    unsigned char aes_key[32];

    hash_data(hash_algo, (unsigned char *) &salted_pin, sizeof(salted_pin), aes_key, NULL);

    // Send the salt and get the server cert. This doesn't have a read timeout
    // because the user must enter the PIN before the server responds
    construct_url(hnd, url, sizeof(url), false, server->serverInfo.address, "pair",
                  "devicename=roth&updateState=1&phrase=getservercert&salt=%s&clientcert=%s", salt_hex, hnd->cert_hex);
    data = http_create_data();
    assert(data);

    if ((ret = http_request(hnd->http, url, data)) != GS_OK) {
        gs_error = "Failed to request pairing. Check connection.";
        goto cleanup;
    }

    if ((ret = xml_status(data->memory, data->size)) != GS_OK) {
        gs_error = "Host returned malformed data.";
        goto cleanup;
    } else if ((ret = xml_search(data->memory, data->size, "paired", &result)) != GS_OK) {
        gs_error = "Host returned incorrect response.";
        goto cleanup;
    }

    if (strcmp(result, "1") != 0) {
        gs_error = "Pairing was declined.";
        ret = GS_FAILED;
        goto cleanup;
    }

    free(result);
    result = NULL;
    // Save this cert for retrieval later
    if ((ret = xml_search(data->memory, data->size, "plaincert", &result)) != GS_OK) {
        // Attempting to pair while another device is pairing will cause GFE
        // to give an empty cert in the response.
        gs_error = "Failed to parse plaincert";
        ret = GS_FAILED;
        goto cleanup;
    }

    if (strlen(result) / 2 > 8191) {
        gs_error = "Server certificate too big";
        ret = GS_FAILED;
        goto cleanup;
    }

    unsigned char server_cert_str[8192];
    size_t plaincert_len = 0;
    hex_to_bytes(result, server_cert_str, sizeof(server_cert_str) - 1, &plaincert_len);
    server_cert_str[plaincert_len] = 0;

    if ((ret = mbedtls_x509_crt_parse(&server_cert, server_cert_str, plaincert_len + 1)) != 0) {
        char errstr[4096];
        mbedtls_strerror(ret, errstr, 4096);
        printf("%s\n", errstr);
        gs_error = "Failed to parse server cert";
        ret = GS_FAILED;
        goto cleanup;
    }

    mbedtls_aes_setkey_enc(&aes_enc, aes_key, 128);
    mbedtls_aes_setkey_dec(&aes_dec, aes_key, 128);

    // Generate a random challenge and encrypt it with our AES key
    unsigned char random_challenge[16];
    mbedtls_ctr_drbg_random(&ctr_drbg, random_challenge, 16);

    unsigned char encrypted_challenge[16];
    if (!crypt_data(&aes_enc, MBEDTLS_AES_ENCRYPT, random_challenge, encrypted_challenge, 16)) {
        gs_error = "Failed to encrypt random_challenge";
        ret = GS_FAILED;
        goto cleanup;
    }

    char encrypted_challenge_hex[33];
    bytes_to_hex(encrypted_challenge, encrypted_challenge_hex, 16);

    // Send the encrypted challenge to the server
    construct_url(hnd, url, sizeof(url), false, server->serverInfo.address, "pair",
                  "devicename=roth&updateState=1&clientchallenge=%s", encrypted_challenge_hex);
    if ((ret = http_request(hnd->http, url, data)) != GS_OK)
        goto cleanup;

    free(result);
    result = NULL;
    if ((ret = xml_status(data->memory, data->size) != GS_OK))
        goto cleanup;
    else if ((ret = xml_search(data->memory, data->size, "paired", &result)) != GS_OK)
        goto cleanup;

    if (strcmp(result, "1") != 0) {
        gs_error = "Failed pairing at stage #2";
        ret = GS_FAILED;
        goto cleanup;
    }

    free(result);
    result = NULL;
    // Decode the server's response and subsequent challenge
    if (xml_search(data->memory, data->size, "challengeresponse", &result) != GS_OK) {
        ret = GS_INVALID;
        goto cleanup;
    }

    unsigned char enc_server_challenge_response[128];
    size_t challenge_resp_len;
    hex_to_bytes(result, enc_server_challenge_response, sizeof(enc_server_challenge_response), &challenge_resp_len);

    unsigned char dec_server_challenge_response[128];
    crypt_data(&aes_dec, MBEDTLS_AES_DECRYPT, enc_server_challenge_response, dec_server_challenge_response,
               challenge_resp_len);

    // Using another 16 bytes secret, compute a challenge response hash using the secret, our cert sig, and the challenge
    unsigned char client_secret[16];
    mbedtls_ctr_drbg_random(&ctr_drbg, client_secret, 16);

    struct challenge_response_t challenge_response;
    memcpy(challenge_response.challenge, &dec_server_challenge_response[hash_length],
           sizeof(challenge_response.challenge));

#if MBEDTLS_VERSION_NUMBER >= 0x03020100
    memcpy(challenge_response.signature, hnd->cert.private_sig.p, sizeof(challenge_response.signature));
#else
    memcpy(challenge_response.signature, hnd->cert.sig.p, sizeof(challenge_response.signature));
#endif
    memcpy(challenge_response.secret, client_secret, sizeof(challenge_response.secret));

    unsigned char challenge_response_hash[32];
    memset(challenge_response_hash, 0, sizeof(challenge_response_hash));
    hash_data(hash_algo, (unsigned char *) &challenge_response, sizeof(struct challenge_response_t),
              challenge_response_hash, NULL);

    unsigned char challenge_response_encrypted[32];
    crypt_data(&aes_enc, MBEDTLS_AES_ENCRYPT, challenge_response_hash, challenge_response_encrypted, 32);

    char challenge_response_hex[65];
    bytes_to_hex(challenge_response_encrypted, challenge_response_hex, 32);

    construct_url(hnd, url, sizeof(url), false, server->serverInfo.address, "pair",
                  "devicename=rothupdateState=1&serverchallengeresp=%s", challenge_response_hex);
    if ((ret = http_request(hnd->http, url, data)) != GS_OK)
        goto cleanup;

    free(result);
    result = NULL;
    if ((ret = xml_status(data->memory, data->size) != GS_OK))
        goto cleanup;
    else if ((ret = xml_search(data->memory, data->size, "paired", &result)) != GS_OK)
        goto cleanup;

    if (strcmp(result, "1") != 0) {
        gs_error = "Failed pairing at stage #3";
        ret = GS_FAILED;
        goto cleanup;
    }

    free(result);
    result = NULL;
    // Get the server's signed secret
    if (xml_search(data->memory, data->size, "pairingsecret", &result) != GS_OK) {
        ret = GS_INVALID;
        goto cleanup;
    }

    struct pairing_secret_t pairing_secret;
    hex_to_bytes(result, (unsigned char *) &pairing_secret, sizeof(pairing_secret), NULL);

    // Ensure the authenticity of the data
    if (!verifySignature(pairing_secret.secret, sizeof(pairing_secret.secret),
                         pairing_secret.signature, sizeof(pairing_secret.signature), &server_cert)) {
        gs_error = "MITM attack detected";
        ret = GS_FAILED;
        goto cleanup;
    }

    // Ensure the server challenge matched what we expected (aka the PIN was correct)
    struct challenge_response_t expected_response_data;
    memcpy(expected_response_data.challenge, random_challenge, 16);
#if MBEDTLS_VERSION_NUMBER >= 0x03020100
    memcpy(expected_response_data.signature, server_cert.private_sig.p, 256);
#else
    memcpy(expected_response_data.signature, server_cert.sig.p, 256);
#endif
    memcpy(expected_response_data.secret, pairing_secret.secret, 16);

    unsigned char expected_hash[32];
    hash_data(hash_algo, (unsigned char *) &expected_response_data, sizeof(expected_response_data),
              expected_hash, NULL);
    if (memcmp(expected_hash, dec_server_challenge_response, hash_length) != 0) {
        // Probably got the wrong PIN
        gs_error = "Incorrect PIN";
        ret = GS_FAILED;
        goto cleanup;
    }

    // Send the server our signed secret
    struct pairing_secret_t client_pairing_secret;
    memcpy(client_pairing_secret.secret, client_secret, 16);
    size_t s_len = sizeof(client_pairing_secret.signature);
    if (!generateSignature(client_pairing_secret.secret, 16, client_pairing_secret.signature, &s_len,
                           (struct mbedtls_pk_context *) &hnd->pk, &ctr_drbg)) {
        gs_error = "Failed to sign data";
        ret = GS_FAILED;
        goto cleanup;
    }

    char client_pairing_secret_hex[sizeof(client_pairing_secret) * 2 + 1];
    bytes_to_hex((unsigned char *) &client_pairing_secret, client_pairing_secret_hex, sizeof(client_pairing_secret));

    construct_url(hnd, url, sizeof(url), false, server->serverInfo.address, "pair",
                  "devicename=roth&updateState=1&clientpairingsecret=%s", client_pairing_secret_hex);
    if ((ret = http_request(hnd->http, url, data)) != GS_OK)
        goto cleanup;

    free(result);
    result = NULL;
    if ((ret = xml_status(data->memory, data->size) != GS_OK))
        goto cleanup;
    else if ((ret = xml_search(data->memory, data->size, "paired", &result)) != GS_OK)
        goto cleanup;

    if (strcmp(result, "1") != 0) {
        gs_error = "Failed pairing at stage #4";
        ret = GS_FAILED;
        goto cleanup;
    }

    // Do the initial challenge (seems neccessary for us to show as paired)
    construct_url(hnd, url, sizeof(url), true, server->serverInfo.address, "pair",
                  "devicename=roth&updateState=1&phrase=pairchallenge");
    if ((ret = http_request(hnd->http, url, data)) != GS_OK)
        goto cleanup;

    free(result);
    result = NULL;
    if ((ret = xml_status(data->memory, data->size) != GS_OK))
        goto cleanup;
    else if ((ret = xml_search(data->memory, data->size, "paired", &result)) != GS_OK)
        goto cleanup;

    if (strcmp(result, "1") != 0) {
        gs_error = "Failed pairing at stage #5";
        ret = GS_FAILED;
        goto cleanup;
    }

    server->paired = true;

    cleanup:
    if (ret != GS_OK)
        gs_unpair(hnd, server);

    if (result != NULL)
        free(result);

    if (data != NULL)
        http_free_data(data);

    mbedtls_aes_free(&aes_enc);
    mbedtls_aes_free(&aes_dec);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_x509_crt_free(&server_cert);

    return ret;
}

int gs_applist(GS_CLIENT hnd, const SERVER_DATA *server, PAPP_LIST *list) {
    int ret = GS_OK;
    char url[4096];
    PHTTP_DATA data = http_create_data();
    if (data == NULL)
        return GS_OUT_OF_MEMORY;

    construct_url(hnd, url, sizeof(url), true, server->serverInfo.address, "applist", NULL);
    if (http_request(hnd->http, url, data) != GS_OK)
        ret = GS_IO_ERROR;
    else if (xml_status(data->memory, data->size) == GS_ERROR)
        ret = GS_ERROR;
    else if (xml_applist(data->memory, data->size, list) != GS_OK)
        ret = GS_INVALID;

    http_free_data(data);
    return ret;
}

int gs_start_app(GS_CLIENT hnd, PSERVER_DATA server, STREAM_CONFIGURATION *config, int appId, bool sops,
                 bool localaudio, int gamepad_mask) {
    int ret = GS_OK;
    char *result = NULL;
    PHTTP_DATA data = NULL;

    mbedtls_entropy_context entropy;
    mbedtls_entropy_init(&entropy);

    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ctr_drbg_init(&ctr_drbg);

    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0)) != 0) {
        goto cleanup;
    }

    PDISPLAY_MODE mode = server->modes;
    bool correct_mode = false;
    while (mode != NULL) {
        if (mode->width == config->width && mode->height == config->height) {
            if (mode->refresh == config->fps)
                correct_mode = true;
        }

        mode = mode->next;
    }

    if (!correct_mode && !server->unsupported)
        return GS_NOT_SUPPORTED_MODE;

    if (config->height >= 2160 && !server->supports4K)
        return GS_NOT_SUPPORTED_4K;

    mbedtls_ctr_drbg_random(&ctr_drbg, (unsigned char *) config->remoteInputAesKey, 16);
    memset(config->remoteInputAesIv, 0, 16);
    mbedtls_ctr_drbg_random(&ctr_drbg, (unsigned char *) config->remoteInputAesIv, 4);
    uint32_t rikeyid = 0;
    memcpy(&rikeyid, config->remoteInputAesIv, 4);
    rikeyid = htonl(rikeyid);

    char url[4096];
    char rikey_hex[33];
    bytes_to_hex((unsigned char *) config->remoteInputAesKey, rikey_hex, 16);

    data = http_create_data();
    assert(data);

    int surround_info = SURROUNDAUDIOINFO_FROM_AUDIO_CONFIGURATION(config->audioConfiguration);
    if (server->currentGame == 0) {
        // Using an FPS value over 60 causes SOPS to default to 720p60,
        // so force it to 0 to ensure the correct resolution is set. We
        // used to use 60 here but that locked the frame rate to 60 FPS
        // on GFE 3.20.3.
        int fps = config->fps > 60 ? 0 : config->fps;
        construct_url(hnd, url, sizeof(url), true, server->serverInfo.address, "launch",
                      "appid=%d&mode=%dx%dx%d&additionalStates=1&sops=%d&rikey=%s&rikeyid=%d&localAudioPlayMode=%d&surroundAudioInfo=%d&remoteControllersBitmap=%d&gcmap=%d",
                      appId, config->width, config->height, fps, sops, rikey_hex, rikeyid, localaudio, surround_info,
                      gamepad_mask, gamepad_mask);
    } else {
        construct_url(hnd, url, sizeof(url), true, server->serverInfo.address, "resume",
                      "rikey=%s&rikeyid=%d&surroundAudioInfo=%d", rikey_hex, rikeyid, surround_info);
    }
    if ((ret = http_request(hnd->http, url, data)) == GS_OK)
        server->currentGame = appId;
    else
        goto cleanup;

    if ((ret = xml_status(data->memory, data->size) != GS_OK))
        goto cleanup;
    else if ((ret = xml_search(data->memory, data->size, "gamesession", &result)) != GS_OK)
        goto cleanup;

    if (!strcmp(result, "0")) {
        ret = GS_FAILED;
        goto cleanup;
    }

    if (xml_search(data->memory, data->size, "sessionUrl0", &result) == GS_OK) {
        server->serverInfo.rtspSessionUrl = result;
        result = NULL;
    }

    cleanup:
    if (result != NULL)
        free(result);

    if (data != NULL)
        http_free_data(data);

    mbedtls_ctr_drbg_free(&ctr_drbg);
    return ret;
}

int gs_quit_app(GS_CLIENT hnd, PSERVER_DATA server) {
    int ret = GS_OK;
    char url[4096];
    char *result = NULL;
    PHTTP_DATA data = http_create_data();
    if (data == NULL)
        return GS_OUT_OF_MEMORY;

    construct_url(hnd, url, sizeof(url), true, server->serverInfo.address, "cancel", NULL);
    if ((ret = http_request(hnd->http, url, data)) != GS_OK) {
        goto cleanup;
    }

    if ((ret = xml_status(data->memory, data->size) != GS_OK)) {
        goto cleanup;
    } else if ((ret = xml_search(data->memory, data->size, "cancel", &result)) != GS_OK) {
        goto cleanup;
    }

    if (strcmp(result, "0") == 0) {
        ret = GS_FAILED;
        goto cleanup;
    }
    if (ret == GS_OK) {
        server->currentGame = 0;
    }

    cleanup:
    if (result != NULL)
        free(result);

    http_free_data(data);
    return ret;
}

int gs_download_cover(GS_CLIENT hnd, const SERVER_DATA *server, int appid, const char *path) {
    int ret = GS_OK;
    char url[4096];
    PHTTP_DATA data = http_create_data();
    if (data == NULL)
        return GS_OUT_OF_MEMORY;

    construct_url(hnd, url, sizeof(url), true, server->serverInfo.address, "appasset",
                  "appid=%d&AssetType=2&AssetIdx=0", appid);
    ret = http_request(hnd->http, url, data);
    if (ret != GS_OK)
        goto cleanup;

    FILE *f = fopen(path, "wb");
    if (!f) {
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

GS_CLIENT gs_new(const char *keydir, int log_level) {
    struct GS_CLIENT_T *hnd = malloc(sizeof(struct GS_CLIENT_T));
    memset(hnd, 0, sizeof(struct GS_CLIENT_T));
    mkdirtree(keydir);
    if (load_unique_id(hnd, keydir) != GS_OK) {
        free(hnd);
        return NULL;
    }

    if (load_cert(hnd, keydir) != GS_OK) {
        free(hnd);
        return NULL;
    }

    hnd->http = http_create(keydir, log_level);
    gs_set_timeout(hnd, 5);
    return hnd;
}

void gs_destroy(GS_CLIENT hnd) {
    mbedtls_pk_free(&hnd->pk);
    mbedtls_x509_crt_free(&hnd->cert);
    http_destroy(hnd->http);
    free((void *) hnd);
}

void gs_set_timeout(GS_CLIENT hnd, int timeout) {
    http_set_timeout(hnd->http, timeout);
}

int gs_init(GS_CLIENT hnd, PSERVER_DATA server, const char *address, bool unsupported) {
    LiInitializeServerInformation(&server->serverInfo);
    server->serverInfo.address = address;
    server->unsupported = unsupported;
    return load_server_status(hnd, server);
}

static bool construct_url(GS_CLIENT hnd, char *url, size_t ulen, bool secure, const char *address, const char *action,
                          const char *fmt, ...) {
    char uuid_str[37];
#if __WIN32
    UUID uuid;
    UuidCreate(&uuid);
    RPC_CSTR szUuid = NULL;
    if (UuidToString(&uuid, &szUuid) != RPC_S_OK) {
        return false;
    }
    strncpy(uuid_str, (char *) szUuid, sizeof(uuid_str));
    RpcStringFree(&szUuid);
#else
    uuid_t uuid;
    uuid_generate_random(uuid);
    uuid_unparse(uuid, uuid_str);
#endif

    int port = secure ? 47984 : 47989;
    char *const proto = secure ? "https" : "http";
    if (fmt) {
        char params[4096];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(params, 4096, fmt, ap);
        va_end(ap);
        snprintf(url, ulen, "%s://%s:%d/%s?uniqueid=%s&uuid=%.*s&%s", proto, address, port, action,
                 hnd->unique_id, 36, uuid_str, params);
    } else {
        snprintf(url, ulen, "%s://%s:%d/%s?uniqueid=%s&uuid=%.*s", proto, address, port,
                 action, hnd->unique_id, 36, uuid_str);
    }
    return true;
}
