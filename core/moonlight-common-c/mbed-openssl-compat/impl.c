#include "openssl/evp.h"
#include "openssl/rand.h"

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>

int mbed_RAND_bytes(unsigned char *buf, int num)
{
    mbedtls_entropy_context entropy;
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ctr_drbg_init(&ctr_drbg);

    int result = 0;
    if (mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0) != 0)
    {
        goto cleanup;
    }

    mbedtls_ctr_drbg_random(&ctr_drbg, buf, num);
    result = num;

cleanup:
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    return num;
}

EVP_CIPHER_CTX *mbed_EVP_CIPHER_CTX_new(void)
{
    return NULL;
}

int mbed_EVP_CIPHER_CTX_ctrl(EVP_CIPHER_CTX *ctx, int type, int arg, void *ptr)
{
    return 0;
}

int mbed_EVP_EncryptInit_ex(EVP_CIPHER_CTX *ctx,
                       const EVP_CIPHER cipher, ENGINE *impl,
                       const unsigned char *key,
                       const unsigned char *iv)
{
    return 0;
}

int mbed_EVP_EncryptUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out,
                      int *outl, const unsigned char *in, int inl)
{
    return 0;
}

int mbed_EVP_EncryptFinal_ex(EVP_CIPHER_CTX *ctx, unsigned char *out,
                        int *outl)
{
    return 0;
}

int mbed_EVP_CIPHER_CTX_reset(EVP_CIPHER_CTX *c)
{
    return 0;
}

void mbed_EVP_CIPHER_CTX_free(EVP_CIPHER_CTX *c)
{
}

const EVP_CIPHER mbed_EVP_aes_128_gcm(void)
{
    return 0;
}

const EVP_CIPHER mbed_EVP_aes_128_cbc(void)
{
    return 0;
}