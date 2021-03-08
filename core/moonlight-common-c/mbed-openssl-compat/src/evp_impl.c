#include "evp.h"

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/gcm.h>
#include <mbedtls/error.h>

struct EVP_CIPHER_CTX_T
{
    mbedtls_gcm_context ctx;
    mbedtls_cipher_type_t cipher;
    int ivlen;
};

static int failed_logging(int ret);

EVP_CIPHER_CTX *mbed_EVP_CIPHER_CTX_new(void)
{
    EVP_CIPHER_CTX *ctx = malloc(sizeof(EVP_CIPHER_CTX));
    ctx->cipher = MBEDTLS_CIPHER_NONE;
    ctx->ivlen = 0;
    mbedtls_gcm_init(&ctx->ctx);
    return ctx;
}

int mbed_EVP_CIPHER_CTX_ctrl(EVP_CIPHER_CTX *ctx, int type, int arg, void *ptr)
{
    switch (type)
    {
    case EVP_CTRL_GCM_GET_TAG:
        return mbedtls_gcm_finish(&ctx->ctx, (unsigned char *)ptr, arg) == 0 ? 1 : 0;
    case EVP_CTRL_GCM_SET_IVLEN:
        ctx->ivlen = arg;
        return 1;
    default:
        break;
    }
    return 0;
}

int mbed_EVP_EncryptInit_ex(EVP_CIPHER_CTX *ctx,
                            const EVP_CIPHER cipher, ENGINE *impl,
                            const unsigned char *key,
                            const unsigned char *iv)
{
    if (!key || !iv)
    {
        ctx->cipher = cipher;
        return 1;
    }
    int ret;
    if ((ret = mbedtls_gcm_setkey(&ctx->ctx, MBEDTLS_CIPHER_ID_AES, key, 128)) != 0)
        return failed_logging(ret);
    if ((ret = mbedtls_gcm_starts(&ctx->ctx, MBEDTLS_GCM_ENCRYPT, iv, ctx->ivlen, NULL, 0)) != 0)
        return failed_logging(ret);
    return 1;
}

int mbed_EVP_EncryptUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out,
                           int *outl, const unsigned char *in, int inl)
{
    int ret;
    if ((ret = mbedtls_gcm_update(&ctx->ctx, inl, in, out)) != 0)
        return failed_logging(ret);
    *outl = 16;
    return 1;
}

int mbed_EVP_EncryptFinal_ex(EVP_CIPHER_CTX *ctx, unsigned char *out,
                             int *outl)
{
    *outl = 0;
    return 1;
}

int mbed_EVP_CIPHER_CTX_reset(EVP_CIPHER_CTX *c)
{
    mbedtls_gcm_free(&c->ctx);
    mbedtls_gcm_init(&c->ctx);
    return 1;
}

void mbed_EVP_CIPHER_CTX_free(EVP_CIPHER_CTX *c)
{
    mbedtls_gcm_free(&c->ctx);
    free(c);
}

const EVP_CIPHER mbed_EVP_aes_128_gcm(void)
{
    return MBEDTLS_CIPHER_AES_128_GCM;
}

const EVP_CIPHER mbed_EVP_aes_128_cbc(void)
{
    return MBEDTLS_CIPHER_AES_128_CBC;
}

int failed_logging(int ret)
{
    char buf[1024];
    mbedtls_strerror(ret, buf, 1024);
    fprintf(stderr, "mbedtls error: %d - %s\n", ret, buf);
    return ret;
}