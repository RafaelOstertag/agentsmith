
/* Copyright (C) 2010, 2011 by Rafael Ostertag
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

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif

#ifdef HAVE_OPENSSL_SSL_H
#include <openssl/ssl.h>
#endif

#ifdef HAVE_OPENSSL_ERR_H
#include <openssl/err.h>
#endif

#ifdef HAVE_OPENSSL_BIO_H
#include <openssl/bio.h>
#endif

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include<string.h>

#include "globals.h"
#include "netssl.h"
#include "output.h"
#include "sslutils.h"
#include "cfg.h"

static int ssl_initialized = 0;
static SSL_CTX *ssl_ctx_client = NULL;
static SSL_CTX *ssl_ctx_server = NULL;

int
netssl_initialize() {
    assert(ssl_initialized == 0);
    assert(ssl_ctx_client == NULL);
    assert(ssl_ctx_server == NULL);

    SSL_load_error_strings();
    ERR_load_BIO_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();

    initialize_locking();

    if (CONFIG.inform) {
        /*
         * Check if the necessary files have been specified 
         */
        if (strlen(CONFIG.ssl_ca_file) < 1) {
            out_err("No CA file specified");
            exit(1);
        }
        if (strlen(CONFIG.ssl_crl_file) < 1) {
            out_err("No CRL file specified");
            exit(1);
        }
        if (strlen(CONFIG.ssl_client_cert) < 1) {
            out_err("No client certificate specified");
            exit(1);
        }
        if (strlen(CONFIG.ssl_client_key) < 1) {
            out_err("No client key specified");
            exit(1);
        }

#if OPENSSL_VERSION_NUMBER  < 0x10100000
        ssl_ctx_client = SSL_CTX_new(TLSv1_2_client_method());
#else
        ssl_ctx_client = SSL_CTX_new(TLS_client_method());
#endif
        if (ssl_ctx_client == NULL) {
            out_err("Unable to initialize SSL Context for client (%s)",
                    ERR_reason_error_string(ERR_get_error()));
            exit(1);
        }

        /*
         * Set verification stuff 
         */
        client_verification(ssl_ctx_client);
    }

    if (CONFIG.server) {
        /*
         * Check if the necessary files have been specified 
         */
        if (strlen(CONFIG.ssl_ca_file) < 1) {
            out_err("No CA file specified");
            exit(1);
        }
        if (strlen(CONFIG.ssl_crl_file) < 1) {
            out_err("No CRL file specified");
            exit(1);
        }
        if (strlen(CONFIG.ssl_server_cert) < 1) {
            out_err("No server certificate specified");
            exit(1);
        }
        if (strlen(CONFIG.ssl_server_key) < 1) {
            out_err("No server key specified");
            exit(1);
        }

#if OPENSSL_VERSION_NUMBER  < 0x10100000
        ssl_ctx_client = SSL_CTX_new(TLSv1_2_client_method());
#else
        ssl_ctx_server = SSL_CTX_new(TLS_server_method());
#endif
        if (ssl_ctx_server == NULL) {
            out_err("Unable to initialize SSL Context for server (%s)",
                    ERR_reason_error_string(ERR_get_error()));
            exit(1);
        }

        /*
         * Set verification stuff 
         */
        server_verification(ssl_ctx_server);
    }

    ssl_initialized = 1;

    return RETVAL_OK;
}

void
netssl_disintegrate() {


    assert(ssl_initialized != 0);

    if (CONFIG.inform) {
        assert(ssl_ctx_client != NULL);

        SSL_CTX_free(ssl_ctx_client);
        ssl_ctx_client = NULL;
    }

    if (CONFIG.server) {
        assert(ssl_ctx_server != NULL);

        SSL_CTX_free(ssl_ctx_server);
        ssl_ctx_server = NULL;
    }

    deinitialize_locking();

    ERR_free_strings();
}

/**
 * Serves an SSL connection. It takes the socket fd and returns the SSL
 * and BIO object used to talk to the client.
 *
 * @param sockfd the socket
 *
 * @param ssl the SSL object returned
 *
 * @param sbio the BIO object used to talk to the client
 *
 * @return on success it return RETVAL_OK, else RETVAL_ERR.
 */
int
netssl_server(int sockfd, SSL ** ssl, BIO ** sbio) {
    int retval, errnosav;

    assert(ssl != NULL);
    assert(*ssl == NULL);
    assert(sbio != NULL);
    assert(*sbio == NULL);
    assert(sockfd >= 0);
    assert(ssl_ctx_server != NULL);

    *ssl = SSL_new(ssl_ctx_server);
    if (*ssl == NULL) {
        out_err("Thread [%li]: Error creating a SSL context: %s",
                pthread_self(), ERR_reason_error_string(ERR_get_error()));
        *sbio = NULL;
        return RETVAL_ERR;
    }

    /*
     * Don't want to be bothered with renegotation 
     */
    SSL_set_mode(*ssl, SSL_MODE_AUTO_RETRY);

    *sbio = BIO_new_socket(sockfd, BIO_NOCLOSE);
    if (*sbio == NULL) {
        out_err("Thread [%li]: Error creating new socket: %s",
                pthread_self(), ERR_reason_error_string(ERR_get_error()));
        SSL_free(*ssl);
        *ssl = NULL;
        return RETVAL_ERR;
    }

    SSL_set_bio(*ssl, *sbio, *sbio);

    ERR_clear_error();

    retval = SSL_accept(*ssl);
    errnosav = errno;
    if (retval != 1) {
        netssl_ssl_error_to_string(SSL_get_error(*ssl, retval), errnosav);
        SSL_free(*ssl);
        *ssl = NULL;

        /*
         * We don't need to free sbio, SSL_free does take care of this
         */
        return RETVAL_ERR;
    }

    return RETVAL_OK;
}

/**
 * Establishes an SSL connection. It takes the socket fd and returns the SSL
 * and BIO object used to talk to the client.
 *
 * @param sockfd the socket
 *
 * @param ssl the SSL object returned
 *
 * @param sbio the BIO object used to talk to the server
 *
 * @return on success it return RETVAL_OK, else RETVAL_ERR.
 */
int
netssl_client(int sockfd, SSL ** ssl, BIO ** sbio) {
    int retval, errnosav;

    assert(ssl != NULL);
    assert(*ssl == NULL);
    assert(sbio != NULL);
    assert(*sbio == NULL);
    assert(sockfd >= 0);
    assert(ssl_ctx_client != NULL);

    *ssl = SSL_new(ssl_ctx_client);
    if (*ssl == NULL) {
        out_err("Thread [%li]: Error creating a SSL context: %s",
                pthread_self(), ERR_reason_error_string(ERR_get_error()));
        *sbio = NULL;
        return RETVAL_ERR;
    }

    /*
     * Don't want to be bothered with renegotation 
     */
    SSL_set_mode(*ssl, SSL_MODE_AUTO_RETRY);

    *sbio = BIO_new_socket(sockfd, BIO_NOCLOSE);
    if (*sbio == NULL) {
        out_err("Thread [%li]: Error creating new socket: %s",
                pthread_self(), ERR_reason_error_string(ERR_get_error()));
        SSL_free(*ssl);
        *ssl = NULL;
        return RETVAL_ERR;
    }

    SSL_set_bio(*ssl, *sbio, *sbio);

    ERR_clear_error();

    retval = SSL_connect(*ssl);
    errnosav = errno;
    if (retval != 1) {
        netssl_ssl_error_to_string(SSL_get_error(*ssl, retval), errnosav);
        SSL_free(*ssl);
        *ssl = NULL;

        /*
         * We don't need to free sbio, SSL_free does apparently take care of
         * this
         */
        *sbio = NULL;
        return RETVAL_ERR;
    }

    return RETVAL_OK;
}

void
netssl_ssl_error_to_string(int err, int syserr) {
    const char *sslerrmsg;

    switch (err) {
        case SSL_ERROR_NONE:
            out_msg("Thread [%li]: No SSL error", pthread_self());
            break;
        case SSL_ERROR_ZERO_RETURN:
            out_msg("Thread [%li]: SSL connection closed", pthread_self());
            break;
        case SSL_ERROR_WANT_READ:
            out_err("Thread [%li]: SSL read did not complete. Try again",
                    pthread_self());
            break;
        case SSL_ERROR_WANT_WRITE:
            out_err("Thread [%li]: SSL write did not complete. Try again",
                    pthread_self());
            break;
        case SSL_ERROR_WANT_CONNECT:
            out_err("Thread [%li]: SSL connect did not complete. Try again",
                    pthread_self());
            break;
        case SSL_ERROR_WANT_ACCEPT:
            out_err("Thread [%li]: SSL accept did not complete. Try again",
                    pthread_self());
            break;
        case SSL_ERROR_WANT_X509_LOOKUP:
            out_err("Thread [%li]: SSL X509 lookup did not complete. Try again",
                    pthread_self());
            break;
        case SSL_ERROR_SYSCALL:
            out_syserr(syserr, "Thread [%li]: SSL system call error",
                    pthread_self());
            break;
        case SSL_ERROR_SSL:
            sslerrmsg = ERR_reason_error_string(ERR_get_error());
            out_err("Thread [%li]: SSL error (%s)",
                    pthread_self(), sslerrmsg == NULL ? "<NULL>" : sslerrmsg);
            break;
    }
}
