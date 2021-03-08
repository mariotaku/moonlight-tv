#include "rand.h"

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
    return result;
}