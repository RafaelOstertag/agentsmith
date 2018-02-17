/* Copyright (C) 2018 by Rafael Ostertag
 *
 * This file is part of agentsmith.
 *
 * agentsmith is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * agentsmith is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * agentsmith.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holder give permission
 * to link the code of portions of this program with the OpenSSL library under
 * certain conditions as described in each individual source file, and
 * distribute linked combinations including the two.
 *
 * You must obey the GNU General Public License in all respects for all of the
 * code used other than OpenSSL.  If you modify file(s) with this exception,
 * you may extend this exception to your version of the file(s), but you are
 * not obligated to do so.  If you do not wish to do so, delete this exception
 * statement from your version.  If you delete this exception statement from
 * all source files in the program, then also delete it here.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif

#ifdef HAVE_OPENSSL_SSL_H
#include <openssl/ssl.h>
#endif

#ifdef HAVE_OPENSSL_BIO_H
#include <openssl/bio.h>
#endif

#ifdef HAVE_OPENSSL_ERR_H
#include <openssl/err.h>
#endif

#include <assert.h>
#include <errno.h>

#include "globals.h"
#include "output.h"
#include "config.h"
#include "cfg.h"

#if OPENSSL_VERSION_NUMBER  < 0x10100000
static unsigned long
_sslutils_thread_id() {
    return (unsigned long) pthread_self();
}

/*
 * Used for locking
 */
static pthread_mutex_t *lock_cs;
static long *lock_count;

void
_sslutils_locking_callback(int mode, int type, char *file, int line) {
    if (mode & CRYPTO_LOCK) {
        pthread_mutex_lock(&(lock_cs[type]));
        lock_count[type]++;
    } else {
        pthread_mutex_unlock(&(lock_cs[type]));
    }
}
#endif

void initialize_locking() {
#if OPENSSL_VERSION_NUMBER  < 0x10100000
    int retval, i;

    /*
     * Initialize locking
     */
    lock_cs = calloc(CRYPTO_num_locks(), sizeof (pthread_mutex_t));
    if (lock_cs == NULL) {
        out_err("Memory exhausted. Dying");
        exit(1);
    }
    lock_count = calloc(CRYPTO_num_locks(), sizeof (long));
    if (lock_count == NULL) {
        out_err("Memory exhausted. Dying");
        exit(1);
    }

    for (i = 0; i < CRYPTO_num_locks(); i++) {
        retval = pthread_mutex_init(&(lock_cs[i]), NULL);
        if (retval != 0) {
            out_syserr(retval, "Unable to initialize mutex #%i for SSL", i);

            exit(1);
        }
    }

    CRYPTO_set_id_callback((unsigned long (*)()) _sslutils_thread_id);
    CRYPTO_set_locking_callback((void (*)()) _sslutils_locking_callback);
#endif    
}

void deinitialize_locking() {
#if OPENSSL_VERSION_NUMBER  < 0x10100000
    int retval, i;

    for (i = 0; i < CRYPTO_num_locks(); i++) {
        retval = pthread_mutex_destroy(&(lock_cs[i]));
        if (retval != 0)
            out_syserr(retval, "Error destroying SSL mutex #%i", i);

        out_dbg("SSL disintegrate: %8ld:%s", lock_count[i],
                CRYPTO_get_lock_name(i));
    }

    free(lock_cs);
    free(lock_count);
#endif
}

static int _sslutils_verify_callback(int ok, X509_STORE_CTX * ctx) {
    X509 *err_cert;
    int err;
    char peer_cn[BUFFSIZE];

    err_cert = X509_STORE_CTX_get_current_cert(ctx);
    err = X509_STORE_CTX_get_error(ctx);

    if (err_cert) {
        X509_NAME_get_text_by_NID(X509_get_subject_name(err_cert),
                NID_commonName, peer_cn, BUFFSIZE);

        out_dbg("Thread [%li]: Certificate: %s", pthread_self(), peer_cn);
    } else
        out_err("SSL: <no cert>");

    if (!ok) {
        out_err("Thread [%li]: SSL: verify error:num=%d:%s",
                pthread_self(), err, X509_verify_cert_error_string(err));
    }

    switch (err) {
        case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
            out_err("Thread [%li]: SSL: Unable to get issuer cert '%s'",
                    pthread_self(), X509_get_issuer_name(err_cert));
            break;
        case X509_V_ERR_CERT_NOT_YET_VALID:
        case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
            out_err("Thread [%li]: SSL: Not valid before %s",
                    pthread_self(), X509_get_notBefore(err_cert));
            break;
        case X509_V_ERR_CERT_HAS_EXPIRED:
        case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
            out_err("Thread [%li]: SSL: Not valid after %s",
                    pthread_self(), X509_get_notAfter(err_cert));
            break;
        case X509_V_ERR_NO_EXPLICIT_POLICY:
            break;
    }

    out_dbg("Thread [%li]: verify return:%d",
            pthread_self(), ok);
    return (ok);

}

static X509_CRL *_load_crl() {
    X509_CRL *x = NULL;
    BIO *in = NULL;

    in = BIO_new(BIO_s_file());
    if (in == NULL) {
        out_err("Error in BIO_new()");
        goto end;
    }

    if (BIO_read_filename(in, CONFIG.ssl_crl_file) <= 0) {
        out_syserr(errno, "BIO_read_filename(): Unable to load file %s", CONFIG.ssl_crl_file);
        goto end;
    }

    x = PEM_read_bio_X509_CRL(in, NULL, NULL, NULL);
    if (x == NULL) {
        out_syserr(errno, "PEM_read_bio_X509_CRL(): Unable to load file %s", CONFIG.ssl_crl_file);
        goto end;
    }

end:
    BIO_free(in);
    return (x);
}

/**
 * Add a CRL to the SSL CTX. This function expects the SSL_CTX having initialized the X509_STORE already.
 * 
 * @param ssl_ctx pointer to SSL_CTX
 */
static void _add_crl_to_ssl_ctx(SSL_CTX *ssl_ctx) {
    X509_STORE *store;
    X509_VERIFY_PARAM *param;
    X509_CRL *crl;

    assert(ssl_ctx != NULL);

    crl = _load_crl();
    if (!crl) {
        out_err("Unable to load CRL");
        exit(1);
    }

    // Enable CRL checking
    store = SSL_CTX_get_cert_store(ssl_ctx);

    X509_STORE_add_crl(store, crl);

    param = X509_VERIFY_PARAM_new();
    X509_VERIFY_PARAM_set_flags(param, X509_V_FLAG_CRL_CHECK || X509_V_FLAG_CRL_CHECK_ALL);
    X509_STORE_set1_param(store, param);
    X509_VERIFY_PARAM_free(param);
}

void client_verification(SSL_CTX *ctx, const char *ca_file, const char *crl_file) {
    assert(ctx != NULL);
    
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER |
            SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
            _sslutils_verify_callback);

    if (!SSL_CTX_load_verify_locations
            (ctx, ca_file, NULL)) {
        out_err("Unable to load Trust Store for client (%s)",
                ERR_reason_error_string(ERR_get_error()));
        exit(1);
    }

    if (!SSL_CTX_use_certificate_file
            (ctx, CONFIG.ssl_client_cert, SSL_FILETYPE_PEM)) {
        out_err("Unable to load client certificate (%s)",
                ERR_reason_error_string(ERR_get_error()));
        exit(1);
    }

    if (!SSL_CTX_use_PrivateKey_file
            (ctx, CONFIG.ssl_client_key, SSL_FILETYPE_PEM)) {
        out_err("Unable to load client key (%s)",
                ERR_reason_error_string(ERR_get_error()));
        exit(1);
    }

    _add_crl_to_ssl_ctx(ctx);
}

void server_verification(SSL_CTX *ctx) {
    assert(ctx != NULL);
    
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER |
            SSL_VERIFY_FAIL_IF_NO_PEER_CERT |
            SSL_VERIFY_CLIENT_ONCE, _sslutils_verify_callback);

    if (!SSL_CTX_load_verify_locations
            (ctx, CONFIG.ssl_ca_file, NULL)) {
        out_err("Unable to load Trust Store for server (%s)",
                ERR_reason_error_string(ERR_get_error()));
        exit(1);
    }

    if (!SSL_CTX_use_certificate_file
            (ctx, CONFIG.ssl_server_cert, SSL_FILETYPE_PEM)) {
        out_err("Unable to load server certificate (%s)",
                ERR_reason_error_string(ERR_get_error()));
        exit(1);
    }

    if (!SSL_CTX_use_PrivateKey_file
            (ctx, CONFIG.ssl_server_key, SSL_FILETYPE_PEM)) {
        out_err("Unable to load server key (%s)",
                ERR_reason_error_string(ERR_get_error()));
        exit(1);
    }

    _add_crl_to_ssl_ctx(ctx);
}