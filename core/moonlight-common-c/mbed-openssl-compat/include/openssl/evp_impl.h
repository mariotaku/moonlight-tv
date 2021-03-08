#pragma once

#include <mbedtls/cipher.h>

struct EVP_CIPHER_CTX_T;

struct EVP_CIPHER_CTX_T *mbed_EVP_CIPHER_CTX_new(void);

int mbed_EVP_EncryptInit_ex(struct EVP_CIPHER_CTX_T *ctx,
                            const mbedtls_cipher_type_t cipher, void *impl,
                            const unsigned char *key,
                            const unsigned char *iv);

int mbed_EVP_CIPHER_CTX_ctrl(struct EVP_CIPHER_CTX_T *ctx, int type, int arg, void *ptr);

int mbed_EVP_EncryptUpdate(struct EVP_CIPHER_CTX_T *ctx, unsigned char *out,
                           int *outl, const unsigned char *in, int inl);

int mbed_EVP_EncryptFinal_ex(struct EVP_CIPHER_CTX_T *ctx, unsigned char *out,
                             int *outl);

int mbed_EVP_CIPHER_CTX_reset(struct EVP_CIPHER_CTX_T *c);

void mbed_EVP_CIPHER_CTX_free(struct EVP_CIPHER_CTX_T *c);

const mbedtls_cipher_type_t mbed_EVP_aes_128_gcm(void);

const mbedtls_cipher_type_t mbed_EVP_aes_128_cbc(void);