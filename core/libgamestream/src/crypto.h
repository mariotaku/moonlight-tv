#pragma once

#include <stdbool.h>

#include <mbedtls/aes.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/md.h>
#include <mbedtls/pk.h>
#include <mbedtls/sha1.h>
#include <mbedtls/sha256.h>
#include <mbedtls/x509_crt.h>

static int sha1(const unsigned char *input, size_t ilen, unsigned char *output);

static int sha256(const unsigned char *input, size_t ilen, unsigned char *output, int is224);

static int hash_data(mbedtls_md_type_t type, const unsigned char *input, size_t ilen, unsigned char *output, size_t *olen)
{
  switch (type)
  {
  case MBEDTLS_MD_SHA1:
    if (olen)
      *olen = 20;
    return sha1(input, ilen, output);
  case MBEDTLS_MD_SHA256:
    if (olen)
      *olen = 32;
    return sha256(input, ilen, output, 0);
  default:
    return -1;
  }
}

static bool crypt_data(mbedtls_aes_context *ctx, int mode, const unsigned char *input, unsigned char *output, size_t len)
{
  for (int i = 0; i < len; i += 16)
  {
    if (mbedtls_aes_crypt_ecb(ctx, mode, &input[i], &output[i]) != 0)
    {
      return false;
    }
  }
  return true;
}

static bool generateSignature(const unsigned char *msg, size_t mlen, unsigned char *sig, size_t *slen, mbedtls_pk_context *pkey, mbedtls_ctr_drbg_context *rng)
{
  int result = 0;
  unsigned char hash[32];
  if ((result = sha256(msg, mlen, hash, 0)) != 0)
  {
    goto cleanup;
  }
#if MBEDTLS_VERSION_NUMBER >= 0x03020100
  if ((result = mbedtls_pk_sign(pkey, MBEDTLS_MD_SHA256, hash, 32, sig, *slen, slen, mbedtls_ctr_drbg_random, rng)) != 0)
#else
  if ((result = mbedtls_pk_sign(pkey, MBEDTLS_MD_SHA256, hash, 32, sig, slen, mbedtls_ctr_drbg_random, rng)) != 0)
#endif
  {
    goto cleanup;
  }

cleanup:
  return result == 0;
}

static bool verifySignature(const unsigned char *msg, size_t mlen, const unsigned char *sig, size_t slen, mbedtls_x509_crt *cert)
{
  int result = 0;
  unsigned char hash[32];
  if ((result = sha256(msg, mlen, hash, 0)) != 0)
  {
    return false;
  }

  result = mbedtls_pk_verify(&cert->pk, MBEDTLS_MD_SHA256, hash, 32, sig, slen);

  return result == 0;
}

static int sha1(const unsigned char *input, size_t ilen, unsigned char *output) {
#if MBEDTLS_VERSION_NUMBER >= 0x03020100
    return mbedtls_sha1(input, ilen, output);
#else
    return mbedtls_sha1_ret(input, ilen, output);
#endif
}

static int sha256(const unsigned char *input, size_t ilen, unsigned char *output, int is224) {
#if MBEDTLS_VERSION_NUMBER >= 0x03020100
    return mbedtls_sha256(input, ilen, output, is224);
#else
    return mbedtls_sha256_ret(input, ilen, output, is224);
#endif
}