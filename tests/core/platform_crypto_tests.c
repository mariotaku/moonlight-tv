#include "unity.h"
#include "PlatformCrypto.h"

#include <memory.h>

#define AES_KEY { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, \
0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf }
#define ENCRYPTED_DATA { \
0xbb, 0x10, 0x32, 0x66, 0xe9, 0x7f, 0xc9, 0x59, \
0xfb, 0xc6, 0x2e, 0x64, 0xb3, 0x05, 0x0b, 0x94, \
0x00, 0x12, 0x73, 0xfa, 0xfb, 0xaa, 0xec, 0x6b, \
0xc1, 0x15, 0x27, 0x9a, 0xcc, 0x0a, 0x9b, 0x97, \
0x68, 0x01, 0x70, 0x83, 0x8b, 0xf6, 0xb0, 0x23, \
0x64, 0xdb, 0x7b, 0x01, 0x85, 0x6b, 0x97, 0xde, \
0x63, 0xd8, 0xe8, 0x5d, 0xce, 0x25, 0x3e, 0x75, \
0x9e, 0x47, 0xb1, 0xfd, 0x08, 0xbe, 0x7b, 0x2a, \
0xe5, 0x1c, 0xc2, 0x6a, 0x3e, 0xd4, 0xd2, 0x2f, \
0xd0, 0xdf, 0xc5, 0x12, 0xd1, 0x45, 0xe4, 0x44, \
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  \
}
#define ENCRYPTED_DATA_LEN 64

PPLT_CRYPTO_CONTEXT cryptoContext;
unsigned char plainText[64];

void setUp(void) {
    // set stuff up here
    cryptoContext = PltCreateCryptoContext();
    for (int i = 0; i < sizeof(plainText); i++) {
        plainText[i] = i * 4;
    }
}

void tearDown(void) {
    // clean stuff up here
    PltDestroyCryptoContext(cryptoContext);
}

void test_aes_gcm_encrypt(void) {
    unsigned char aesKey[16] = AES_KEY;
    unsigned char iv[16] = {0};
    unsigned char output[128] = {0};
    unsigned char expected[] = ENCRYPTED_DATA;

    size_t tagLength = 16;
    int encryptedLength = sizeof(output) - tagLength;
    memset(output, 0xff, tagLength);

    TEST_ASSERT_TRUE(PltEncryptMessage(cryptoContext, ALGORITHM_AES_GCM, 0, aesKey, sizeof(aesKey), iv, sizeof(iv),
                                       output, tagLength, plainText, sizeof(plainText), output + tagLength,
                                       &encryptedLength));


    TEST_ASSERT_EQUAL(ENCRYPTED_DATA_LEN, encryptedLength);
    TEST_ASSERT_EQUAL_MEMORY(expected, output, sizeof(output));
}

void test_aes_gcm_decrypt(void) {
    unsigned char aesKey[16] = AES_KEY;
    unsigned char iv[16] = {0};
    unsigned char plain[128];
    int plainLength = sizeof(plain);
    size_t tagLength = 16;
    unsigned char encrypted[] = ENCRYPTED_DATA;
    TEST_ASSERT_TRUE(PltDecryptMessage(cryptoContext, ALGORITHM_AES_GCM, 0, aesKey, sizeof(aesKey), iv, sizeof(iv),
                                       encrypted, tagLength, encrypted + tagLength, ENCRYPTED_DATA_LEN,
                                       plain, &plainLength));

    TEST_ASSERT_EQUAL(sizeof(plainText), plainLength);
    TEST_ASSERT_EQUAL_MEMORY(plainText, plain, plainLength);
}

// not needed when using generate_test_runner.rb
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_aes_gcm_encrypt);
    RUN_TEST(test_aes_gcm_decrypt);
    return UNITY_END();
}