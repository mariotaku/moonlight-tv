/*
 * Moonlight is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Moonlight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Moonlight; if not, see <http://www.gnu.org/licenses/>.
 */

#include "mkcert.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <mbedtls/rsa.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/md.h>
#include <mbedtls/error.h>

static const int NUM_BITS = 2048;
static const int SERIAL = 0;
static const int NUM_YEARS = 10;

static int mkcert_generate_impl(mbedtls_pk_context *key, mbedtls_x509write_cert *crt, mbedtls_ctr_drbg_context *rng)
{
    int ret = 0;

    char issuer_name[256];

    mbedtls_mpi serial;

    mbedtls_mpi_init(&serial);

    if ((ret = mbedtls_pk_setup(key, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA))) != 0)
    {
        goto finally;
    }

    if ((ret = mbedtls_rsa_gen_key(mbedtls_pk_rsa(*key), mbedtls_ctr_drbg_random, rng, NUM_BITS, 65537)) != 0)
    {
        goto finally;
    }

    mbedtls_x509write_crt_set_subject_key(crt, key);
    mbedtls_x509write_crt_set_issuer_key(crt, key);

    mbedtls_x509write_crt_set_subject_name(crt, "CN=NVIDIA GameStream Client");
    mbedtls_x509write_crt_set_issuer_name(crt, "CN=NVIDIA GameStream Client");

    mbedtls_mpi_read_string(&serial, 10, "1");
    mbedtls_x509write_crt_set_serial(crt, &serial);

    mbedtls_x509write_crt_set_version(crt, MBEDTLS_X509_CRT_VERSION_2);
    mbedtls_x509write_crt_set_md_alg(crt, MBEDTLS_MD_SHA256);

    time_t now;
    time(&now);
    struct tm *ptr_time;
    ptr_time = gmtime(&now);
    char not_before[16], not_after[16];
    strftime(not_before, 16, "%Y%m%d%H%M%S", ptr_time);
    ptr_time->tm_year += NUM_YEARS;
    strftime(not_after, 16, "%Y%m%d%H%M%S", ptr_time);
    if ((ret = mbedtls_x509write_crt_set_validity(crt, not_before, not_after)) != 0)
    {
        goto finally;
    }

finally:
    mbedtls_mpi_free(&serial);
    return ret;
}

int mkcert_generate(const char *certFile, const char *keyFile)
{
    int ret = 0;
    FILE *fd;
    char buf[4096];
    const char *pers = "GameStream";

    mbedtls_pk_context key;
    mbedtls_x509write_cert crt;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;

    mbedtls_pk_init(&key);
    mbedtls_x509write_crt_init(&crt);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);

    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)pers, strlen(pers))) != 0)
    {
        goto finally;
    }

    if ((ret = mkcert_generate_impl(&key, &crt, &ctr_drbg)) != 0)
    {
        goto finally;
    }

    if ((ret = mbedtls_pk_write_key_pem(&key, buf, 4096)) != 0)
    {
        mbedtls_strerror(ret, buf, 4096);
        printf(" failed\n  !  mbedtls_pk_write_key_pem returned -0x%04x - %s", (unsigned int)-ret, buf);
        goto finally;
    }

    fd = fopen(keyFile, "w");
    fwrite(buf, strlen(buf), 1, fd);
    fflush(fd);
    fclose(fd);

    if ((ret = mbedtls_x509write_crt_pem(&crt, buf, 4096, mbedtls_ctr_drbg_random, &ctr_drbg)) != 0)
    {
        mbedtls_strerror(ret, buf, 4096);
        printf(" failed\n  !  mbedtls_x509write_crt_pem returned -0x%04x - %s", (unsigned int)-ret, buf);
        goto finally;
    }

    fd = fopen(certFile, "w");
    fwrite(buf, strlen(buf), 1, fd);
    fflush(fd);
    fclose(fd);

finally:
    mbedtls_pk_free(&key);
    mbedtls_x509write_crt_free(&crt);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    return ret;
}