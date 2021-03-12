#include <assert.h>
#include <memory.h>

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <mbedtls/aes.h>
#include <mbedtls/cipher.h>
#include <mbedtls/error.h>

#include "crypto.h"

void encrypt_ossl(const unsigned char *input, unsigned char *output, size_t len, const unsigned char *key)
{
    EVP_CIPHER_CTX *cipher;
    int ciphertextLen;

    cipher = EVP_CIPHER_CTX_new();
    assert(cipher);

    EVP_EncryptInit(cipher, EVP_aes_128_ecb(), key, NULL);
    EVP_CIPHER_CTX_set_padding(cipher, 0);

    EVP_EncryptUpdate(cipher, output, &ciphertextLen, input, len);
    assert(ciphertextLen == len);

    EVP_CIPHER_CTX_free(cipher);
}

void encrypt_mbed(const unsigned char *input, unsigned char *output, size_t len, const unsigned char *key)
{
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, key, 128);

    assert(crypt_data(&aes, MBEDTLS_AES_ENCRYPT, input, output, len));

    mbedtls_aes_free(&aes);
}

void decrypt_ossl(const unsigned char *input, unsigned char *output, size_t len, const unsigned char *key)
{
    EVP_CIPHER_CTX *cipher;
    int ciphertextLen;

    cipher = EVP_CIPHER_CTX_new();
    assert(cipher);

    EVP_DecryptInit(cipher, EVP_aes_128_ecb(), key, NULL);
    EVP_CIPHER_CTX_set_padding(cipher, 0);

    EVP_DecryptUpdate(cipher, output, &ciphertextLen, input, len);
    assert(ciphertextLen == len);

    EVP_CIPHER_CTX_free(cipher);
}

void decrypt_mbed(const unsigned char *input, unsigned char *output, size_t len, const unsigned char *key)
{
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, key, 128);

    assert(crypt_data(&aes, MBEDTLS_AES_DECRYPT, input, output, len));

    mbedtls_aes_free(&aes);
}

int main(int argc, char *argv[])
{
    unsigned char input[48], key[16];
    RAND_bytes(input, 48);
    RAND_bytes(key, 16);

    unsigned char output1[48], output2[48];
    for (int i = 16; i <= 48; i += 16)
    {
        encrypt_ossl(input, output1, i, key);
        encrypt_mbed(input, output2, i, key);
        assert(memcmp(output1, output2, i) == 0);
    }

    unsigned char decrypt1[48], decrypt2[48];
    for (int i = 16; i <= 48; i += 16)
    {
        decrypt_ossl(output1, decrypt1, i, key);
        decrypt_mbed(output2, decrypt2, i, key);
        assert(memcmp(decrypt1, decrypt2, i) == 0);
    }
    return 0;
}