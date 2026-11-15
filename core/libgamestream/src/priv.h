#pragma once

#include "http.h"
#include <mbedtls/pk.h>
#include <mbedtls/x509_crt.h>

#define UNIQUE_FILE_NAME "uniqueid.dat"
#define UNIQUE_ID_DEFAULT "0123456789ABCDEF"

#define UNIQUEID_BYTES 8
#define UNIQUEID_CHARS (UNIQUEID_BYTES * 2)

#define DEVICE_NAME_DEFAULT "roth"
#define DEVICE_NAME_MAX_BYTES 64

struct GS_CLIENT_T {
    char unique_id[UNIQUEID_CHARS + 1];
    mbedtls_pk_context pk;
    mbedtls_x509_crt cert;
    char cert_hex[8192];
    /** URL-encoded device name sent when pairing */
    char device_name[DEVICE_NAME_MAX_BYTES * 3 + 1];
    HTTP *http;
};
