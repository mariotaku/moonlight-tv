#include <stdio.h>
#include <assert.h>
#include <memory.h>

#include "evp_impl.h"
#include <openssl/evp.h>

const static unsigned char aesKey[16] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf};
const static unsigned char aesIv[16] = {0x0, 0x1, 0x0, 0x1, 0x0, 0x1, 0x0, 0x1, 0x0, 0x1, 0x0, 0x1, 0x0, 0x1, 0x0, 0x1};

int do_compat(const unsigned char *plaintext, int plaintextLen,
              unsigned char *ciphertext, int *ciphertextLen);
int do_native(const unsigned char *plaintext, int plaintextLen,
              unsigned char *ciphertext, int *ciphertextLen);

int main(int argc, char *argv[])
{
    const char plaintext[16] = "0123456789abcdef";
    char actual_txt[128], expected_txt[128];
    for (int i = 0; i < 10; i++)
    {
        memset(actual_txt, 0, 128);
        memset(expected_txt, 0, 128);
        int actual_len = 0, expected_len = 0;
        assert(do_native(plaintext, 16, expected_txt, &expected_len) == 0);
        assert(do_compat(plaintext, 16, actual_txt, &actual_len) == 0);

        assert(actual_len == expected_len);
        assert(memcmp(actual_txt, expected_txt, actual_len) == 0);
    }
    return 0;
}

int do_compat(const unsigned char *plaintext, int plaintextLen,
              unsigned char *ciphertext, int *ciphertextLen)
{
    int ret;
    int len;

    struct EVP_CIPHER_CTX_T *cipherContext;

    if ((cipherContext = mbed_EVP_CIPHER_CTX_new()) == NULL)
    {
        return -1;
    }

    // Gen 7 servers use 128-bit AES GCM
    if (mbed_EVP_EncryptInit_ex(cipherContext, mbed_EVP_aes_128_gcm(), NULL, NULL, NULL) != 1)
    {
        ret = -1;
        goto gcm_cleanup;
    }

    // Gen 7 servers uses 16 byte IVs
    if (mbed_EVP_CIPHER_CTX_ctrl(cipherContext, EVP_CTRL_GCM_SET_IVLEN, 16, NULL) != 1)
    {
        ret = -1;
        goto gcm_cleanup;
    }

    // Initialize again but now provide our key and current IV
    if (mbed_EVP_EncryptInit_ex(cipherContext, 0, NULL, aesKey, aesIv) != 1)
    {
        ret = -1;
        goto gcm_cleanup;
    }

    // Encrypt into the caller's buffer, leaving room for the auth tag to be prepended
    if (mbed_EVP_EncryptUpdate(cipherContext, &ciphertext[16], ciphertextLen, plaintext, plaintextLen) != 1)
    {
        ret = -1;
        goto gcm_cleanup;
    }

    // GCM encryption won't ever fill ciphertext here but we have to call it anyway
    if (mbed_EVP_EncryptFinal_ex(cipherContext, ciphertext, &len) != 1)
    {
        ret = -1;
        goto gcm_cleanup;
    }
    assert(len == 0);

    // Read the tag into the caller's buffer
    if (mbed_EVP_CIPHER_CTX_ctrl(cipherContext, EVP_CTRL_GCM_GET_TAG, 16, ciphertext) != 1)
    {
        ret = -1;
        goto gcm_cleanup;
    }

    // Increment the ciphertextLen to account for the tag
    *ciphertextLen += 16;

    ret = 0;

gcm_cleanup:
    mbed_EVP_CIPHER_CTX_free(cipherContext);
    return ret;
}

int do_native(const unsigned char *plaintext, int plaintextLen,
              unsigned char *ciphertext, int *ciphertextLen)
{
    int ret;
    int len;

    EVP_CIPHER_CTX *cipherContext;

    if ((cipherContext = EVP_CIPHER_CTX_new()) == NULL)
    {
        return -1;
    }

    // Gen 7 servers use 128-bit AES GCM
    if (EVP_EncryptInit_ex(cipherContext, EVP_aes_128_gcm(), NULL, NULL, NULL) != 1)
    {
        ret = -1;
        goto gcm_cleanup;
    }

    // Gen 7 servers uses 16 byte IVs
    if (EVP_CIPHER_CTX_ctrl(cipherContext, EVP_CTRL_GCM_SET_IVLEN, 16, NULL) != 1)
    {
        ret = -1;
        goto gcm_cleanup;
    }

    // Initialize again but now provide our key and current IV
    if (EVP_EncryptInit_ex(cipherContext, NULL, NULL, aesKey, aesIv) != 1)
    {
        ret = -1;
        goto gcm_cleanup;
    }

    // Encrypt into the caller's buffer, leaving room for the auth tag to be prepended
    if (EVP_EncryptUpdate(cipherContext, &ciphertext[16], ciphertextLen, plaintext, plaintextLen) != 1)
    {
        ret = -1;
        goto gcm_cleanup;
    }

    // GCM encryption won't ever fill ciphertext here but we have to call it anyway
    if (EVP_EncryptFinal_ex(cipherContext, ciphertext, &len) != 1)
    {
        ret = -1;
        goto gcm_cleanup;
    }
    assert(len == 0);

    // Read the tag into the caller's buffer
    if (EVP_CIPHER_CTX_ctrl(cipherContext, EVP_CTRL_GCM_GET_TAG, 16, ciphertext) != 1)
    {
        ret = -1;
        goto gcm_cleanup;
    }

    // Increment the ciphertextLen to account for the tag
    *ciphertextLen += 16;

    ret = 0;

gcm_cleanup:
    EVP_CIPHER_CTX_free(cipherContext);
    return ret;
}