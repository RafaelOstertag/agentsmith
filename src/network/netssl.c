
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

#include "globals.h"
#include "netssl.h"
#include "output.h"
#include "cfg.h"

static int ssl_initialized = 0;
static SSL_CTX *ssl_ctx_client = NULL;
static SSL_CTX *ssl_ctx_server = NULL;

/*
 * Used for locking
 */
static pthread_mutex_t *lock_cs;
static long *lock_count;

void
_netssl_locking_callback(int mode, int type, char *file, int line) {
    if (mode & CRYPTO_LOCK) {
	pthread_mutex_lock(&(lock_cs[type]));
	lock_count[type]++;
    } else {
	pthread_mutex_unlock(&(lock_cs[type]));
    }
}

unsigned long
_netssl_thread_id() {
    return (unsigned long) pthread_self();
}

int
_netssl_verify_callback(int ok, X509_STORE_CTX * ctx) {
    X509     *err_cert;
    int       err;
    char      peer_cn[BUFFSIZE];

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

int
netssl_initialize() {
    int       retval, i;

    assert(ssl_initialized == 0);
    assert(ssl_ctx_client == NULL);
    assert(ssl_ctx_server == NULL);

    SSL_load_error_strings();
    ERR_load_BIO_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();

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

    CRYPTO_set_id_callback((unsigned long (*)()) _netssl_thread_id);
    CRYPTO_set_locking_callback((void (*)()) _netssl_locking_callback);

    if (CONFIG.inform) {
	/*
	 * Check if the necessary files have been specified 
	 */
	if (strlen(CONFIG.ssl_ca_file) < 1) {
	    out_err("No CA file specified");
	    exit(1);
	}
	if (strlen(CONFIG.ssl_client_cert) < 1) {
	    out_err("No client certificate specfied");
	    exit(1);
	}
	if (strlen(CONFIG.ssl_client_key) < 1) {
	    out_err("No client key specified");
	    exit(1);
	}

	ssl_ctx_client = SSL_CTX_new(SSLv3_client_method());
	if (ssl_ctx_client == NULL) {
	    out_err("Unable to initialize SSL Context for client (%s)",
		    ERR_reason_error_string(ERR_get_error()));
	    exit(1);
	}

	/*
	 * Set verification stuff 
	 */
	SSL_CTX_set_verify(ssl_ctx_client, SSL_VERIFY_PEER |
			   SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
			   _netssl_verify_callback);

	if (!SSL_CTX_load_verify_locations
	    (ssl_ctx_client, CONFIG.ssl_ca_file, NULL)) {
	    out_err("Unable to load Trust Store for client (%s)",
		    ERR_reason_error_string(ERR_get_error()));
	    exit(1);
	}

	if (!SSL_CTX_use_certificate_file
	    (ssl_ctx_client, CONFIG.ssl_client_cert, SSL_FILETYPE_PEM)) {
	    out_err("Unable to load client certificate (%s)",
		    ERR_reason_error_string(ERR_get_error()));
	    exit(1);
	}

	if (!SSL_CTX_use_PrivateKey_file
	    (ssl_ctx_client, CONFIG.ssl_client_key, SSL_FILETYPE_PEM)) {
	    out_err("Unable to load client key (%s)",
		    ERR_reason_error_string(ERR_get_error()));
	    exit(1);
	}
    }

    if (CONFIG.server) {
	/*
	 * Check if the necessary files have been specified 
	 */
	if (strlen(CONFIG.ssl_ca_file) < 1) {
	    out_err("No CA file specified");
	    exit(1);
	}
	if (strlen(CONFIG.ssl_server_cert) < 1) {
	    out_err("No server certificate specfied");
	    exit(1);
	}
	if (strlen(CONFIG.ssl_server_key) < 1) {
	    out_err("No server key specified");
	    exit(1);
	}

	ssl_ctx_server = SSL_CTX_new(SSLv3_server_method());
	if (ssl_ctx_server == NULL) {
	    out_err("Unable to initialize SSL Context for server (%s)",
		    ERR_reason_error_string(ERR_get_error()));
	    exit(1);
	}

	/*
	 * Set verification stuff 
	 */
	SSL_CTX_set_verify(ssl_ctx_server, SSL_VERIFY_PEER |
			   SSL_VERIFY_FAIL_IF_NO_PEER_CERT |
			   SSL_VERIFY_CLIENT_ONCE, _netssl_verify_callback);

	if (!SSL_CTX_load_verify_locations
	    (ssl_ctx_server, CONFIG.ssl_ca_file, NULL)) {
	    out_err("Unable to load Trust Store for server (%s)",
		    ERR_reason_error_string(ERR_get_error()));
	    exit(1);
	}

	if (!SSL_CTX_use_certificate_file
	    (ssl_ctx_server, CONFIG.ssl_server_cert, SSL_FILETYPE_PEM)) {
	    out_err("Unable to load server certificate (%s)",
		    ERR_reason_error_string(ERR_get_error()));
	    exit(1);
	}

	if (!SSL_CTX_use_PrivateKey_file
	    (ssl_ctx_server, CONFIG.ssl_server_key, SSL_FILETYPE_PEM)) {
	    out_err("Unable to load server key (%s)",
		    ERR_reason_error_string(ERR_get_error()));
	    exit(1);
	}
    }

    ssl_initialized = 1;

    return RETVAL_OK;
}

void
netssl_disintegrate() {
    int       retval, i;

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

    for (i = 0; i < CRYPTO_num_locks(); i++) {
	retval = pthread_mutex_destroy(&(lock_cs[i]));
	if (retval != 0)
	    out_syserr(retval, "Error destroying SSL mutex #%i", i);

	out_dbg("SSL disintegrate: %8ld:%s", lock_count[i],
		CRYPTO_get_lock_name(i));
    }

    ERR_free_strings();

    free(lock_cs);
    free(lock_count);
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
    int       retval, errnosav;

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
    int       retval, errnosav;

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
