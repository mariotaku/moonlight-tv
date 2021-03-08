#include "evp.h"

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/cipher.h>
#include <mbedtls/error.h>

struct EVP_CIPHER_CTX_T
{
    mbedtls_cipher_context_t ctx;
    mbedtls_cipher_type_t cipher;
    int ivlen;
};

static int failed_logging(int ret);

EVP_CIPHER_CTX *mbed_EVP_CIPHER_CTX_new(void)
{
    EVP_CIPHER_CTX *ctx = malloc(sizeof(EVP_CIPHER_CTX));
    ctx->cipher = MBEDTLS_CIPHER_NONE;
    ctx->ivlen = 0;
    mbedtls_cipher_init(&ctx->ctx);
    return ctx;
}

int mbed_EVP_CIPHER_CTX_ctrl(EVP_CIPHER_CTX *ctx, int type, int arg, void *ptr)
{
    switch (type)
    {
    case EVP_CTRL_GCM_GET_TAG:
    {
        size_t olen;
        return mbedtls_cipher_finish(&ctx->ctx, (unsigned char *)ptr, &olen) == 0 ? 1 : 0;
    }
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
    if ((ret = mbedtls_cipher_setup(&ctx->ctx, mbedtls_cipher_info_from_type(ctx->cipher))) != 0)
        return failed_logging(ret);
    if ((ret = mbedtls_cipher_setkey(&ctx->ctx, key, 128, MBEDTLS_ENCRYPT)) != 0)
        return failed_logging(ret);
    if ((ret = mbedtls_cipher_set_iv(&ctx->ctx, iv, ctx->ivlen)) != 0)
        return failed_logging(ret);
    return 1;
}

int mbed_EVP_EncryptUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out,
                           int *outl, const unsigned char *in, int inl)
{
    return mbedtls_cipher_update(&ctx->ctx, in, inl, out, (size_t *)outl) == 0 ? 1 : 0;
}

int mbed_EVP_EncryptFinal_ex(EVP_CIPHER_CTX *ctx, unsigned char *out,
                             int *outl)
{
    *outl = 0;
    return 1;
}

int mbed_EVP_CIPHER_CTX_reset(EVP_CIPHER_CTX *c)
{
    mbedtls_cipher_free(&c->ctx);
    mbedtls_cipher_init(&c->ctx);
    return 1;
}

void mbed_EVP_CIPHER_CTX_free(EVP_CIPHER_CTX *c)
{
    mbedtls_cipher_free(&c->ctx);
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