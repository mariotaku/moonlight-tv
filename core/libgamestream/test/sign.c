#include <assert.h>
#include <memory.h>

#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/bio.h>
#include <openssl/pem.h>

#include "crypto.h"
#include "mkcert.c"

static int generateSignature_openssl(const char *msg, size_t mlen, unsigned char **sig, size_t *slen, EVP_PKEY *pkey);
static bool verifySignature_openssl(const char *data, int dataLength, char *signature, int signatureLength, const char *cert);

int main(int argc, char *argv[])
{
    int ret = 0;
    FILE *fd;
    char keybuf[4096], crtbuf[4096];
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

    assert(mbedtls_pk_write_key_pem(&key, keybuf, 4096) == 0);
    assert(mbedtls_x509write_crt_pem(&crtw, crtbuf, 4096, mbedtls_ctr_drbg_random, &ctr_drbg) == 0);

    assert(mbedtls_x509_crt_parse(&crt, crtbuf, strlen(crtbuf) + 1) == 0);

    char msg[288];
    assert(mbedtls_ctr_drbg_random(&ctr_drbg, msg, sizeof(msg)) == 0);

    char sig[1024];
    size_t siglen;

    assert(generateSignature(msg, sizeof(msg), sig, &siglen, &key, &ctr_drbg));
    assert(verifySignature_openssl(msg, sizeof(msg), sig, siglen, crtbuf));

    EVP_PKEY *pkey;
    BIO *bio = BIO_new(BIO_s_mem());
    BIO_puts(bio, keybuf);
    PEM_read_bio_PrivateKey(bio, &pkey, NULL, NULL);
    BIO_free(bio);

    unsigned char *sig2;
    assert(generateSignature_openssl(msg, sizeof(msg), &sig2, &siglen, pkey) == 0);
    assert(verifySignature(msg, sizeof(msg), sig2, siglen, &crt));

finally:
    mbedtls_pk_free(&key);
    mbedtls_x509write_crt_free(&crtw);
    mbedtls_x509_crt_free(&crt);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    return ret;
}

static bool verifySignature_openssl(const char *data, int dataLength, char *signature, int signatureLength, const char *cert)
{
    X509 *x509;
    BIO *bio = BIO_new(BIO_s_mem());
    BIO_puts(bio, cert);
    x509 = PEM_read_bio_X509(bio, NULL, NULL, NULL);

    BIO_free(bio);

    if (!x509)
    {
        return false;
    }

    EVP_PKEY *pubKey = X509_get_pubkey(x509);
    EVP_MD_CTX *mdctx = NULL;
    mdctx = EVP_MD_CTX_create();
    EVP_DigestVerifyInit(mdctx, NULL, EVP_sha256(), NULL, pubKey);
    EVP_DigestVerifyUpdate(mdctx, data, dataLength);
    int result = EVP_DigestVerifyFinal(mdctx, signature, signatureLength);

    X509_free(x509);
    EVP_PKEY_free(pubKey);
    EVP_MD_CTX_destroy(mdctx);

    return result > 0;
}

static int generateSignature_openssl(const char *msg, size_t mlen, unsigned char **sig, size_t *slen, EVP_PKEY *pkey)
{
    int result = -1;

    *sig = NULL;
    *slen = 0;

    EVP_MD_CTX *ctx = EVP_MD_CTX_create();
    if (ctx == NULL)
        return -1;

    const EVP_MD *md = EVP_get_digestbyname("SHA256");
    if (md == NULL)
        goto cleanup;

    int rc = EVP_DigestInit_ex(ctx, md, NULL);
    if (rc != 1)
        goto cleanup;

    rc = EVP_DigestSignInit(ctx, NULL, md, NULL, pkey);
    if (rc != 1)
        goto cleanup;

    rc = EVP_DigestSignUpdate(ctx, msg, mlen);
    if (rc != 1)
        goto cleanup;

    size_t req = 0;
    rc = EVP_DigestSignFinal(ctx, NULL, &req);
    if (rc != 1 || !(req > 0))
        goto cleanup;

    *sig = OPENSSL_malloc(req);
    if (*sig == NULL)
        goto cleanup;

    *slen = req;
    rc = EVP_DigestSignFinal(ctx, *sig, slen);
    if (rc != 1 || req != *slen)
        goto cleanup;

    result = 0;

cleanup:
    EVP_MD_CTX_destroy(ctx);
    ctx = NULL;

    return result;
}
