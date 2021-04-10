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
    mbedtls_cipher_type_t cipher;
    unsigned char iv[16];
    int ivlen;
    union
    {
        mbedtls_gcm_context gcm;
        mbedtls_aes_context aes;
    } ctx;
};

static int failed_logging(int ret);

EVP_CIPHER_CTX *mbed_EVP_CIPHER_CTX_new(void)
{
    EVP_CIPHER_CTX *ctx = malloc(sizeof(EVP_CIPHER_CTX));
    ctx->cipher = MBEDTLS_CIPHER_NONE;
    ctx->ivlen = 0;
    return ctx;
}

int mbed_EVP_CIPHER_CTX_ctrl(EVP_CIPHER_CTX *ctx, int type, int arg, void *ptr)
{
    switch (type)
    {
    case EVP_CTRL_GCM_GET_TAG:
        return mbedtls_gcm_finish(&ctx->ctx.gcm, (unsigned char *)ptr, arg) == 0 ? 1 : 0;
    case EVP_CTRL_GCM_SET_TAG:
        return mbedtls_gcm_finish(&ctx->ctx.gcm, (unsigned char *)ptr, arg) == 0 ? 1 : 0;
    case EVP_CTRL_GCM_SET_IVLEN:
        ctx->ivlen = arg;
        return 1;
    default:
        break;
    }
    return 0;
}

int mbed_EVP_CIPHER_CTX_reset(EVP_CIPHER_CTX *c)
{
    switch (c->cipher)
    {
    case MBEDTLS_CIPHER_AES_128_GCM:
        mbedtls_gcm_free(&c->ctx.gcm);
        mbedtls_gcm_init(&c->ctx.gcm);
        break;
    case MBEDTLS_CIPHER_AES_128_CBC:
        mbedtls_aes_free(&c->ctx.aes);
        mbedtls_aes_init(&c->ctx.aes);
        break;
    }
    return 1;
}

void mbed_EVP_CIPHER_CTX_free(EVP_CIPHER_CTX *c)
{
    switch (c->cipher)
    {
    case MBEDTLS_CIPHER_AES_128_GCM:
        mbedtls_gcm_free(&c->ctx.gcm);
        break;
    case MBEDTLS_CIPHER_AES_128_CBC:
        mbedtls_aes_free(&c->ctx.aes);
        break;
    }
    free(c);
}

int mbed_EVP_EncryptInit_ex(EVP_CIPHER_CTX *ctx,
                            const EVP_CIPHER cipher, ENGINE *impl,
                            const unsigned char *key,
                            const unsigned char *iv)
{
    if (cipher)
    {
        ctx->cipher = cipher;
    }
    if (!key || !iv)
    {
        return 1;
    }
    int ret;

    switch (ctx->cipher)
    {
    case MBEDTLS_CIPHER_AES_128_GCM:
        mbedtls_gcm_init(&ctx->ctx.gcm);
        if ((ret = mbedtls_gcm_setkey(&ctx->ctx.gcm, MBEDTLS_CIPHER_ID_AES, key, 128)) != 0)
            return failed_logging(ret);
        memcpy(ctx->iv, iv, ctx->ivlen);
        if ((ret = mbedtls_gcm_starts(&ctx->ctx.gcm, MBEDTLS_GCM_ENCRYPT, iv, ctx->ivlen, NULL, 0)) != 0)
            return failed_logging(ret);
        break;
    case MBEDTLS_CIPHER_AES_128_CBC:
        mbedtls_aes_init(&ctx->ctx.aes);
        if ((ret = mbedtls_aes_setkey_enc(&ctx->ctx.aes, key, 128)) != 0)
            return failed_logging(ret);
        memcpy(ctx->iv, iv, 16);
        break;
    }

    return 1;
}

int mbed_EVP_EncryptUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out,
                           int *outl, const unsigned char *in, int inl)
{
    int ret;

    switch (ctx->cipher)
    {
    case MBEDTLS_CIPHER_AES_128_GCM:
        if ((ret = mbedtls_gcm_update(&ctx->ctx.gcm, inl, in, out)) != 0)
            return failed_logging(ret);
        break;
    case MBEDTLS_CIPHER_AES_128_CBC:
        if ((ret = mbedtls_aes_crypt_cbc(&ctx->ctx.aes, MBEDTLS_AES_ENCRYPT, inl, ctx->iv, in, out)) != 0)
            return failed_logging(ret);
        break;
    }
    *outl = inl;
    return 1;
}

int mbed_EVP_EncryptFinal_ex(EVP_CIPHER_CTX *ctx, unsigned char *out,
                             int *outl)
{
    *outl = 0;
    return 1;
}

int mbed_EVP_DecryptInit_ex(EVP_CIPHER_CTX *ctx,
                            const EVP_CIPHER cipher, ENGINE *impl,
                            const unsigned char *key,
                            const unsigned char *iv)
{
    if (cipher)
    {
        ctx->cipher = cipher;
    }
    if (!key || !iv)
    {
        return 1;
    }
    int ret;

    switch (ctx->cipher)
    {
    case MBEDTLS_CIPHER_AES_128_GCM:
        mbedtls_gcm_init(&ctx->ctx.gcm);
        if ((ret = mbedtls_gcm_setkey(&ctx->ctx.gcm, MBEDTLS_CIPHER_ID_AES, key, 128)) != 0)
            return failed_logging(ret);
        memcpy(ctx->iv, iv, ctx->ivlen);
        if ((ret = mbedtls_gcm_starts(&ctx->ctx.gcm, MBEDTLS_GCM_DECRYPT, iv, ctx->ivlen, NULL, 0)) != 0)
            return failed_logging(ret);
        break;
    case MBEDTLS_CIPHER_AES_128_CBC:
        mbedtls_aes_init(&ctx->ctx.aes);
        if ((ret = mbedtls_aes_setkey_dec(&ctx->ctx.aes, key, 128)) != 0)
            return failed_logging(ret);
        memcpy(ctx->iv, iv, 16);
        break;
    }

    return 1;
}

int mbed_EVP_DecryptUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out,
                           int *outl, const unsigned char *in, int inl)
{
    int ret;

    switch (ctx->cipher)
    {
    case MBEDTLS_CIPHER_AES_128_GCM:
        if ((ret = mbedtls_gcm_update(&ctx->ctx.gcm, inl, in, out)) != 0)
            return failed_logging(ret);
        break;
    case MBEDTLS_CIPHER_AES_128_CBC:
        if ((ret = mbedtls_aes_crypt_cbc(&ctx->ctx.aes, MBEDTLS_AES_DECRYPT, inl, ctx->iv, in, out)) != 0)
            return failed_logging(ret);
        break;
    }
    *outl = inl;
    return 1;
}

int mbed_EVP_DecryptFinal_ex(EVP_CIPHER_CTX *ctx, unsigned char *outm,
                             int *outl)
{
    *outl = 0;
    return 1;
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