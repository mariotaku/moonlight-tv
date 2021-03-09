#include <stdio.h>
#include <assert.h>
#include <memory.h>
#include <stdbool.h>

#include "evp_impl.h"
#include <openssl/rand.h>
#include <openssl/evp.h>

#define MAX_INPUT_PACKET_SIZE 128

#define ROUND_TO_PKCS7_PADDED_LEN(x) ((((x) + 15) / 16) * 16)

const static unsigned char aesKey[16] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf};
const static unsigned char aesIv[16] = {0x0, 0x1, 0x0, 0x1, 0x0, 0x1, 0x0, 0x1, 0x0, 0x1, 0x0, 0x1, 0x0, 0x1, 0x0, 0x1};

int do_compat_gcm(const unsigned char *plaintext, int plaintextLen,
                  unsigned char *ciphertext, int *ciphertextLen);
int do_native_gcm(const unsigned char *plaintext, int plaintextLen,
                  unsigned char *ciphertext, int *ciphertextLen);
int do_compat_cbc(const unsigned char *plaintext, int plaintextLen,
                  unsigned char *ciphertext, int *ciphertextLen);
int do_native_cbc(const unsigned char *plaintext, int plaintextLen,
                  unsigned char *ciphertext, int *ciphertextLen);
int AppVersionQuad[4];

int main(int argc, char *argv[])
{
    const char plaintext[16];
    char actual_txt[128], expected_txt[128];
    for (int i = 0; i < 10; i++)
    {
        RAND_bytes(plaintext, 12);
        memset(actual_txt, 0, 128);
        memset(expected_txt, 0, 128);
        int actual_len = 0, expected_len = 0;
        if (i > 5)
        {
            assert(do_native_gcm(plaintext, 12, expected_txt, &expected_len) == 0);
            assert(do_compat_gcm(plaintext, 12, actual_txt, &actual_len) == 0);
        }
        else
        {
            assert(do_native_cbc(plaintext, 12, expected_txt, &expected_len) == 0);
            assert(do_compat_cbc(plaintext, 12, actual_txt, &actual_len) == 0);
        }

        assert(actual_len == expected_len);
        assert(memcmp(actual_txt, expected_txt, actual_len) == 0);
    }
    return 0;
}

static int addPkcs7PaddingInPlace(unsigned char *plaintext, int plaintextLen)
{
    int i;
    int paddedLength = ROUND_TO_PKCS7_PADDED_LEN(plaintextLen);
    unsigned char paddingByte = (unsigned char)(16 - (plaintextLen % 16));

    for (i = plaintextLen; i < paddedLength; i++)
    {
        plaintext[i] = paddingByte;
    }

    return paddedLength;
}

int do_compat_gcm(const unsigned char *plaintext, int plaintextLen,
              unsigned char *ciphertext, int *ciphertextLen)
{
    static struct EVP_CIPHER_CTX_T *cipherContext = NULL;
    static bool cipherInitialized = false;

    int ret;
    int len;

    if (!cipherInitialized)
    {
        if ((cipherContext = mbed_EVP_CIPHER_CTX_new()) == NULL)
        {
            return -1;
        }
        cipherInitialized = true;
    }

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
    mbed_EVP_CIPHER_CTX_reset(cipherContext);

    return ret;
}

int do_native_gcm(const unsigned char *plaintext, int plaintextLen,
              unsigned char *ciphertext, int *ciphertextLen)
{
    static EVP_CIPHER_CTX *cipherContext = NULL;
    static bool cipherInitialized = false;

    int ret;
    int len;

    if (!cipherInitialized)
    {
        if ((cipherContext = EVP_CIPHER_CTX_new()) == NULL)
        {
            return -1;
        }
        cipherInitialized = true;
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
    EVP_CIPHER_CTX_reset(cipherContext);

    return ret;
}

int do_compat_cbc(const unsigned char *plaintext, int plaintextLen,
                  unsigned char *ciphertext, int *ciphertextLen)
{
    static struct EVP_CIPHER_CTX_T *cipherContext = NULL;
    static bool cipherInitialized = false;

    int ret;
    int len;

    unsigned char paddedData[MAX_INPUT_PACKET_SIZE];
    int paddedLength;

    if (!cipherInitialized)
    {
        if ((cipherContext = mbed_EVP_CIPHER_CTX_new()) == NULL)
        {
            ret = -1;
            goto cbc_cleanup;
        }
        cipherInitialized = true;

        // Prior to Gen 7, 128-bit AES CBC is used for encryption
        if (mbed_EVP_EncryptInit_ex(cipherContext, mbed_EVP_aes_128_cbc(), NULL, aesKey, aesIv) != 1)
        {
            ret = -1;
            goto cbc_cleanup;
        }
    }

    // Pad the data to the required block length
    memcpy(paddedData, plaintext, plaintextLen);
    paddedLength = addPkcs7PaddingInPlace(paddedData, plaintextLen);

    if (mbed_EVP_EncryptUpdate(cipherContext, ciphertext, ciphertextLen, paddedData, paddedLength) != 1)
    {
        ret = -1;
        goto cbc_cleanup;
    }

    ret = 0;

cbc_cleanup:
    // Nothing to do
    ;
    return ret;
}

int do_native_cbc(const unsigned char *plaintext, int plaintextLen,
                  unsigned char *ciphertext, int *ciphertextLen)
{
    static EVP_CIPHER_CTX *cipherContext = NULL;
    static bool cipherInitialized = false;

    int ret;
    int len;

    unsigned char paddedData[MAX_INPUT_PACKET_SIZE];
    int paddedLength;

    if (!cipherInitialized)
    {
        if ((cipherContext = EVP_CIPHER_CTX_new()) == NULL)
        {
            ret = -1;
            goto cbc_cleanup;
        }
        cipherInitialized = true;

        // Prior to Gen 7, 128-bit AES CBC is used for encryption
        if (EVP_EncryptInit_ex(cipherContext, EVP_aes_128_cbc(), NULL, aesKey, aesIv) != 1)
        {
            ret = -1;
            goto cbc_cleanup;
        }
    }

    // Pad the data to the required block length
    memcpy(paddedData, plaintext, plaintextLen);
    paddedLength = addPkcs7PaddingInPlace(paddedData, plaintextLen);

    if (EVP_EncryptUpdate(cipherContext, ciphertext, ciphertextLen, paddedData, paddedLength) != 1)
    {
        ret = -1;
        goto cbc_cleanup;
    }

    ret = 0;

cbc_cleanup:
    // Nothing to do
    ;
    return ret;
}