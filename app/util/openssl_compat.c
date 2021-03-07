#include <mbedtls/ctr_drbg.h>

typedef void EVP_CIPHER_CTX;
typedef void ENGINE;
typedef int EVP_CIPHER;

int RAND_bytes(unsigned char *buf, int num)
{
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_ctr_drbg_random(&ctr_drbg, buf, num);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    return num;
}

EVP_CIPHER_CTX *EVP_CIPHER_CTX_new(void)
{
    return NULL;
}

int EVP_CIPHER_CTX_ctrl(EVP_CIPHER_CTX *ctx, int type, int arg, void *ptr)
{
    return 0;
}

int EVP_EncryptInit_ex(EVP_CIPHER_CTX *ctx,
                       const EVP_CIPHER *cipher, ENGINE *impl,
                       const unsigned char *key,
                       const unsigned char *iv)
{
    return 0;
}

int EVP_EncryptUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out,
                      int *outl, const unsigned char *in, int inl)
{
    return 0;
}

int EVP_EncryptFinal_ex(EVP_CIPHER_CTX *ctx, unsigned char *out,
                        int *outl)
{
    return 0;
}

int EVP_CIPHER_CTX_reset(EVP_CIPHER_CTX *c)
{
    return 0;
}

void EVP_CIPHER_CTX_free(EVP_CIPHER_CTX *c)
{
}

const EVP_CIPHER EVP_aes_128_gcm(void)
{
    return 0;
}

const EVP_CIPHER EVP_aes_128_cbc(void)
{
    return 0;
}