/* Copyright (C) 2010, 2011 Rafael Ostertag
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
 */

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
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

#include "globals.h"
#include "netssl.h"
#include "output.h"
#include "cfg.h"

static int ssl_initialized = 0;
static SSL_CTX *ssl_ctx_client;
static SSL_CTX *ssl_ctx_server;

int
netssl_initialize() {
    int retval;

    if (ssl_initialized != 0) return RETVAL_OK;
    
    SSL_load_error_strings();
    ERR_load_BIO_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();

    ssl_ctx_client = SSL_CTX_new(SSLv3_client_method());
    if ( ssl_ctx_client == NULL ) {
	out_err("Unable to initialize SSL Context for client (%s)",
		ERR_reason_error_string(ERR_get_error()));
	exit(1);
    }

    ssl_ctx_server = SSL_CTX_new(SSLv3_server_method());
    if ( ssl_ctx_client == NULL ) {
	out_err("Unable to initialize SSL Context for server (%s)",
		ERR_reason_error_string(ERR_get_error()));
	exit(1);
    }

    

    if (! SSL_CTX_load_verify_locations(ssl_ctx_client, CONFIG.ssl_ca_trust, NULL)) {
	out_err("Unable to load Trust Store for client (%s)",
		ERR_reason_error_string(ERR_get_error()));
	exit(1);
    }

    if (! SSL_CTX_load_verify_locations(ssl_ctx_server, CONFIG.ssl_ca_trust, NULL)) {
	out_err("Unable to load Trust Store for server (%s)",
		ERR_reason_error_string(ERR_get_error()));
	exit(1);
    }

    ssl_initialized = 1;

    return RETVAL_OK;
}

void
netssl_disintegrate() {
    
    if (ssl_initialized == 0) return;

    SSL_CTX_free(ssl_ctx_client);
    SSL_CTX_free(ssl_ctx_server);
}
