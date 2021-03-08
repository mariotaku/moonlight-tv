#pragma once

#define EVP_CTRL_GCM_SET_IVLEN 0x9
#define EVP_CTRL_GCM_GET_TAG 0x10
#define OPENSSL_VERSION_NUMBER 0x10200000L

typedef void EVP_CIPHER_CTX;
typedef void ENGINE;
typedef int EVP_CIPHER;

EVP_CIPHER_CTX *mbed_EVP_CIPHER_CTX_new(void);

int mbed_EVP_EncryptInit_ex(EVP_CIPHER_CTX *ctx,
                            const EVP_CIPHER cipher, ENGINE *impl,
                            const unsigned char *key,
                            const unsigned char *iv);

int mbed_EVP_CIPHER_CTX_ctrl(EVP_CIPHER_CTX *ctx, int type, int arg, void *ptr);

int mbed_EVP_EncryptUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out,
                           int *outl, const unsigned char *in, int inl);

int mbed_EVP_EncryptFinal_ex(EVP_CIPHER_CTX *ctx, unsigned char *out,
                             int *outl);

int mbed_EVP_CIPHER_CTX_reset(EVP_CIPHER_CTX *c);

void mbed_EVP_CIPHER_CTX_free(EVP_CIPHER_CTX *c);

const EVP_CIPHER mbed_EVP_aes_128_gcm(void);

const EVP_CIPHER mbed_EVP_aes_128_cbc(void);

#define EVP_CIPHER_CTX_new() mbed_EVP_CIPHER_CTX_new()
#define EVP_EncryptInit_ex(ctx, cipher, impl, key, iv) mbed_EVP_EncryptInit_ex(ctx, cipher, impl, key, iv)
#define EVP_CIPHER_CTX_ctrl(ctx, type, arg, ptr) mbed_EVP_CIPHER_CTX_ctrl(ctx, type, arg, ptr)
#define EVP_EncryptUpdate(ctx, out, outl, in, inl) mbed_EVP_EncryptUpdate(ctx, out, outl, in, inl)
#define EVP_EncryptFinal_ex(ctx, out, outl) mbed_EVP_EncryptFinal_ex(ctx, out, outl)
#define EVP_CIPHER_CTX_reset(c) mbed_EVP_CIPHER_CTX_reset(c)
#define EVP_CIPHER_CTX_free(c) mbed_EVP_CIPHER_CTX_free(c)

#define EVP_aes_128_gcm() mbed_EVP_aes_128_gcm()
#define EVP_aes_128_cbc() mbed_EVP_aes_128_cbc()