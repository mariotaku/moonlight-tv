#include <assert.h>
#include <memory.h>

#include <openssl/rand.h>
#include <openssl/sha.h>
#include "crypto.h"

int main(int argc, char *argv[])
{
    unsigned char input[48];
    RAND_bytes(input, sizeof(input));

    unsigned char output1[32], output2[32];

    SHA1(input, sizeof(input), output1);
    hash_data(MBEDTLS_MD_SHA1, input, sizeof(input), output2, NULL);
    assert(memcmp(output1, output2, 20) == 0);

    SHA256(input, sizeof(input), output1);
    hash_data(MBEDTLS_MD_SHA256, input, sizeof(input), output2, NULL);
    assert(memcmp(output1, output2, 32) == 0);
    return 0;
}