
/* Copyright (C) 2011 Rafael Ostertag
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
#include <config.h>
#endif

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "globals.h"
#include "client.h"
#include "clientqueue.h"
#include "records.h"
#include "output.h"

enum {
    MAXCLIENTQUEUES = 256,
    HOSTRECORDVECTORLEN = 25
};

static int client_started = 0;

static client_queue_t *queues[MAXCLIENTQUEUES];

/* This is the temporary vector of host records */
static hostrecord_t *temp_hr_vector[HOSTRECORDVECTORLEN];

/* This is the fill of the temporary vector of host records */
static int temp_hr_vector_fill = 0;

int
client_start(const addrinfo_list_t *aglist) {
    int       i;
    const addrinfo_list_t *ptr;

    assert(aglist != NULL);

    memset(queues, 0, sizeof (client_queue_t *) * MAXCLIENTQUEUES);
    memset(temp_hr_vector, 0, sizeof (hostrecord_t *) * HOSTRECORDVECTORLEN);
    temp_hr_vector_fill = 0;

    ptr = aglist;
    for (i = 0; i < MAXCLIENTQUEUES && ptr != 0; i++) {
	queues[i] =
	    client_queue_init(ptr->addr->ai_family, ptr->addr->ai_addr,
			      ptr->addr->ai_addrlen);
	if (queues[i] == NULL) {
	    out_err("Client: error creating new queue");
	    return RETVAL_ERR;
	}
	ptr = ptr->next;
    }

    client_started = 1;
    return RETVAL_OK;
}

/**
 * This function will add the host record to the temporary host record
 * vector. If there is no room for adding another host record,
 * client_queue_flush() is called.
 */
int
client_queue_record(const hostrecord_t *hr) {
    int       retval;

    assert(hr != NULL);
    assert(strlen(hr->ipaddr) > 2);

    if (client_started == 0) {
	return RETVAL_ERR;
    }

    if (temp_hr_vector_fill >= HOSTRECORDVECTORLEN) {
	out_dbg("Client queue flush forced");
	retval = client_queue_flush();
	if (retval == RETVAL_ERR) {
	    return RETVAL_ERR;
	}

	/*
	 * LEAVE FUNCTION 
	 */
	return client_queue_record(hr);
    }

    temp_hr_vector[temp_hr_vector_fill] =
	(hostrecord_t *) malloc(sizeof (hostrecord_t));
    if (temp_hr_vector[temp_hr_vector_fill] == NULL) {
	out_err("Insufficient memory. Dying now");
	exit(1);
    }
    memcpy(temp_hr_vector[temp_hr_vector_fill], hr, sizeof (hostrecord_t));
    assert(strlen(temp_hr_vector[temp_hr_vector_fill]->ipaddr) > 2);

    ++temp_hr_vector_fill;

    return RETVAL_OK;
}

/**
 * This flushes the temporary host record vector to the client worker queues.
 */
int
client_queue_flush() {
    int       i;

    assert(temp_hr_vector_fill <= HOSTRECORDVECTORLEN);

    if (client_started == 0) {
	out_dbg("client_queue_flush(): client not started.");
	/*
	 * This prevents the action thread from spewing error messages when
	 * flushing at startup and the client is not ready. Leave it this
	 * way. 
	 */
	return RETVAL_OK;
    }
    if (temp_hr_vector_fill < 1)
	return RETVAL_OK;

    for (i = 0; i < MAXCLIENTQUEUES && queues[i] != NULL; i++) {
        client_queue_append(queues[i],
				(const hostrecord_t **) temp_hr_vector,
				temp_hr_vector_fill);
	if (queues[i] == NULL) {
	    out_err("Client: error appending to client worker queue");
	    return RETVAL_ERR;
	}
    }

    for (i = 0; i < temp_hr_vector_fill; i++)
	free(temp_hr_vector[i]);

    memset(temp_hr_vector, 0, sizeof (hostrecord_t *) * HOSTRECORDVECTORLEN);
    temp_hr_vector_fill = 0;
    return RETVAL_OK;
}

int
client_stop() {
    int       i;

    for (i = 0; i < MAXCLIENTQUEUES && queues[i] != 0; i++) {
	client_queue_destroy(queues[i]);
	if (queues[i] == NULL)
	    out_err("Client: Appending to client worker queue");
    }

    for (i = 0; i < temp_hr_vector_fill; i++)
	free(temp_hr_vector[i]);

    client_started = 0;
    return RETVAL_OK;
}
