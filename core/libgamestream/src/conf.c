#include "conf.h"
#include "client.h"
#include "errors.h"
#include "set_error.h"
#include "priv.h"
#include "mkcert.h"
#include "http.h"
#include "logging.h"

#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <mbedtls/version.h>
#include <mbedtls/pk.h>
#include <mbedtls/error.h>

#if MBEDTLS_VERSION_NUMBER >= 0x03020100

#include <mbedtls/ctr_drbg.h>

#endif

#if __WIN32

#include <windows.h>

#define PATH_SEPARATOR '\\'
#define MKDIR(path) mkdir(path)
#else

#include <sys/stat.h>

#define MKDIR(path) mkdir(path, 0775)
#define PATH_SEPARATOR '/'
#endif

static int mbed_parse_key(mbedtls_pk_context *ctx, const char *path);

static int mkdirtree(const char *directory);

static int load_unique_id(struct GS_CLIENT_T *hnd, const char *keydir);

static int init_unique_id(const char *keydir);

static int load_cert(struct GS_CLIENT_T *hnd, const char *keydir);

static int init_cert(const char *keydir);

int gs_conf_load(GS_CLIENT hnd, const char *keydir) {
    int ret = GS_OK;
    if ((ret = load_unique_id(hnd, keydir)) != GS_OK) {
        return ret;
    }

    if ((ret = load_cert(hnd, keydir)) != GS_OK) {
        return ret;
    }
    return ret;
}

int gs_conf_init(const char *keydir) {
    commons_log_info("GameStream", "Initializing configuration");
    if (mkdirtree(keydir) != 0) {
        return gs_set_error(GS_IO_ERROR, "Failed to create config directory %s: %s", keydir, strerror(errno));
    }
    int ret;
    if ((ret = init_unique_id(keydir)) != GS_OK) {
        return ret;
    }
    if ((ret = init_cert(keydir)) != GS_OK) {
        return ret;
    }
    commons_log_info("GameStream", "Configuration initialized");
    return ret;
}

int mbed_parse_key(mbedtls_pk_context *ctx, const char *path) {
#if MBEDTLS_VERSION_NUMBER >= 0x03020100
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ctr_drbg_init(&ctr_drbg);
    int ret = mbedtls_pk_parse_keyfile(ctx, path, NULL, mbedtls_ctr_drbg_random, &ctr_drbg);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    return ret;
#else
    return mbedtls_pk_parse_keyfile(ctx, path, NULL);
#endif
}

int load_unique_id(struct GS_CLIENT_T *hnd, const char *keydir) {
    char id_path[PATH_MAX];
    snprintf(id_path, PATH_MAX, "%s%c%s", keydir, PATH_SEPARATOR, UNIQUE_FILE_NAME);

    FILE *fd = fopen(id_path, "r");
    if (fd == NULL) {
        return gs_set_error(GS_BAD_CONF, "Failed to open unique ID file %s: %s", id_path, strerror(errno));
    }
    if (fread(hnd->unique_id, 1, UNIQUEID_CHARS, fd) != UNIQUEID_CHARS) {
        fclose(fd);
        return gs_set_error(GS_BAD_CONF, "Bad unique ID file");
    }
    fclose(fd);
    hnd->unique_id[UNIQUEID_CHARS] = 0;
    return GS_OK;
}

int init_unique_id(const char *keydir) {
    commons_log_info("GameStream", "Generating device unique ID");
    char id_path[PATH_MAX];
    snprintf(id_path, PATH_MAX, "%s%c%s", keydir, PATH_SEPARATOR, UNIQUE_FILE_NAME);
    FILE *fd = fopen(id_path, "w");
    if (fd == NULL) {
        return gs_set_error(GS_BAD_CONF, "Failed to create unique ID file %s: %s", id_path, strerror(errno));
    }
    if (fwrite(UNIQUE_ID_DEFAULT, 1, UNIQUEID_CHARS, fd) != UNIQUEID_CHARS) {
        int ret = gs_set_error(GS_BAD_CONF, "Failed to write unique ID file: %s", strerror(errno));
        fclose(fd);
        return ret;
    }
    fclose(fd);
    return GS_OK;
}

int load_cert(struct GS_CLIENT_T *hnd, const char *keydir) {
    char cert_path[PATH_MAX];
    snprintf(cert_path, PATH_MAX, "%s%c%s", keydir, PATH_SEPARATOR, CERTIFICATE_FILE_NAME);

    char key_path[PATH_MAX];
    snprintf(key_path, PATH_MAX, "%s%c%s", keydir, PATH_SEPARATOR, KEY_FILE_NAME);

    int ret;
    mbedtls_x509_crt_init(&hnd->cert);
    if ((ret = mbedtls_x509_crt_parse_file(&hnd->cert, cert_path)) != 0) {
        char buf[512];
        mbedtls_strerror(ret, buf, 512);
        mbedtls_x509_crt_free(&hnd->cert);
        return gs_set_error(GS_FAILED, "Failed to parse certificate: %s", buf);
    }
    FILE *f = fopen(cert_path, "r");
    if (f == NULL) {
        mbedtls_x509_crt_free(&hnd->cert);
        return gs_set_error(GS_IO_ERROR, "Failed to open certFile %s for reading", cert_path);
    }
    int c;
    int length = 0;
    while ((c = fgetc(f)) != EOF) {
        sprintf(&hnd->cert_hex[length], "%02x", c);
        length += 2;
    }
    hnd->cert_hex[length] = 0;

    fclose(f);

    mbedtls_pk_init(&hnd->pk);
    if ((ret = mbed_parse_key(&hnd->pk, key_path)) != 0) {
        char buf[512];
        mbedtls_strerror(ret, buf, 512);
        mbedtls_x509_crt_free(&hnd->cert);
        mbedtls_pk_free(&hnd->pk);
        return gs_set_error(GS_FAILED, "Error loading key into memory: %s", buf);
    }

    return GS_OK;
}

static int init_cert(const char *keydir) {
    commons_log_info("GameStream", "Generating device cert/key pair");
    char cert_path[PATH_MAX];
    snprintf(cert_path, PATH_MAX, "%s%c%s", keydir, PATH_SEPARATOR, CERTIFICATE_FILE_NAME);

    char key_path[PATH_MAX];
    snprintf(key_path, PATH_MAX, "%s%c%s", keydir, PATH_SEPARATOR, KEY_FILE_NAME);
    return mkcert_generate(cert_path, key_path);
}

int mkdirtree(const char *directory) {
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
