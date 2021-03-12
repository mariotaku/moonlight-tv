#include <assert.h>
#include <memory.h>

#include <openssl/rand.h>
#include <openssl/sha.h>

#include "crypto.h"
#include "mkcert.c"

int main(int argc, char *argv[])
{
    int ret = 0;
    FILE *fd;
    char crtbuf[4096];
    const char *pers = "GameStream";

    mbedtls_pk_context key;
    mbedtls_x509write_cert crtw;
    mbedtls_x509_crt crt;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;

    mbedtls_pk_init(&key);
    mbedtls_x509write_crt_init(&crtw);
    mbedtls_x509_crt_init(&crt);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);

    assert(mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)pers, strlen(pers)) == 0);

    assert(mkcert_generate_impl(&key, &crtw, &ctr_drbg) == 0);

    assert(mbedtls_x509write_crt_pem(&crtw, crtbuf, 4096, mbedtls_ctr_drbg_random, &ctr_drbg) == 0);

    assert(mbedtls_x509_crt_parse(&crt, crtbuf, strlen(crtbuf) + 1) == 0);

    char msg[288];
    assert(mbedtls_ctr_drbg_random(&ctr_drbg, msg, sizeof(msg)) == 0);

    char sig[1024];
    size_t siglen;

    assert(generateSignature(msg, sizeof(msg), sig, &siglen, &key, &ctr_drbg));

    assert(verifySignature(msg, sizeof(msg), sig, siglen, &crt));

finally:
    mbedtls_pk_free(&key);
    mbedtls_x509write_crt_free(&crtw);
    mbedtls_x509_crt_free(&crt);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    return ret;
}