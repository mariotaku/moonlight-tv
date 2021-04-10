#pragma once

#include "evp_impl.h"

typedef struct EVP_CIPHER_CTX_T EVP_CIPHER_CTX;
typedef void ENGINE;
typedef mbedtls_cipher_type_t EVP_CIPHER;

#define EVP_CTRL_GCM_SET_IVLEN 0x9
#define EVP_CTRL_GCM_GET_TAG 0x10
#define EVP_CTRL_GCM_SET_TAG 0x11
#define OPENSSL_VERSION_NUMBER 0x10200000L

#define EVP_CIPHER_CTX_new() mbed_EVP_CIPHER_CTX_new()
#define EVP_CIPHER_CTX_ctrl(ctx, type, arg, ptr) mbed_EVP_CIPHER_CTX_ctrl(ctx, type, arg, ptr)
#define EVP_CIPHER_CTX_reset(c) mbed_EVP_CIPHER_CTX_reset(c)
#define EVP_CIPHER_CTX_free(c) mbed_EVP_CIPHER_CTX_free(c)

#define EVP_EncryptInit_ex(ctx, cipher, impl, key, iv) mbed_EVP_EncryptInit_ex(ctx, (EVP_CIPHER)cipher, impl, key, iv)
#define EVP_EncryptUpdate(ctx, out, outl, in, inl) mbed_EVP_EncryptUpdate(ctx, out, outl, in, inl)
#define EVP_EncryptFinal_ex(ctx, out, outl) mbed_EVP_EncryptFinal_ex(ctx, out, outl)

#define EVP_DecryptInit_ex(ctx, cipher, impl, key, iv) mbed_EVP_DecryptInit_ex(ctx, (EVP_CIPHER)cipher, impl, key, iv)
#define EVP_DecryptUpdate(ctx, out, outl, in, inl) mbed_EVP_DecryptUpdate(ctx, out, outl, in, inl)
#define EVP_DecryptFinal_ex(ctx, outm, outl) mbed_EVP_DecryptFinal_ex(ctx, outm, outl)

#define EVP_aes_128_gcm() mbed_EVP_aes_128_gcm()
#define EVP_aes_128_cbc() mbed_EVP_aes_128_cbc()