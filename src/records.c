/* Copyright (C) 2010 Rafael Ostertag
 *
 * This file is part of sshsentinel.
 *
 * sshsentinel is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Foobar is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#ifdef HAVE_MATH_H
#include <math.h>
#endif

#include "output.h"
#include "records.h"
#include "cfg.h"

enum {
    RETVAL_OK = 0,
    RETVAL_ERR = -1
};

static hostrecord_t **hr_vector = NULL;
/* The chunk size we pre-allocate */
static unsigned long hr_vector_chunksize = 100;
/* This will be dynamically expanded as needed */
static unsigned long hr_vector_size = 100;
static unsigned long hr_vector_fill = 0;

pthread_mutex_t vector_mutex;

static int initialized = 0;

#ifdef DEBUG
unsigned long records_dbg_get_vector_size() { return hr_vector_size; }
unsigned long records_dbg_get_vector_chunksize() { return hr_vector_chunksize; }
unsigned long records_dbg_get_vector_fill() { return hr_vector_fill; }
#endif

/*
 * @return
 * The index of the first free slot in the newly allocated vector.
 */
static unsigned long
_records_vector_grow () {
    unsigned long previous_size;

    previous_size = hr_vector_size;
    hr_vector_size += hr_vector_chunksize;

    hr_vector = realloc((void*) hr_vector,
			sizeof(hostrecord_t**) * hr_vector_size);
    if ( hr_vector == NULL ) {
	out_err("Unable to allocate more space for hr_vector. Fatal");
	exit (3);
    }
    memset (hr_vector + previous_size, 0, sizeof(hostrecord_t**) * hr_vector_chunksize);

    return previous_size;
}

static hostrecord_t*
_records_find(const char* ipaddr, unsigned long *pos) {
    hostrecord_t **ptr;
    unsigned long i;

    ptr = hr_vector;
    for (i = 0; i < hr_vector_fill; i++ ) {
	if ( ptr[i] != NULL ) {
	    if (strcmp(ptr[i]->ipaddr, ipaddr) == 0 ) {
		if ( pos != NULL )
		    *pos = i;
		return ptr[i];
	    }
	}
    }
    return NULL;
}

/**
 * Finds a free slot in the vector.
 *
 * @param nospace pointer to an integer.
 *
 * @return
 *
 * It returns the position of the free slot. If no free slot is found, nospace
 * is set to 1 and 0 is returned.
 */
static unsigned long
_records_find_free(int *nospace) {
    hostrecord_t **ptr;
    unsigned long i;

    assert ( nospace != NULL );

    ptr = hr_vector;
    for (i = 0; i < hr_vector_size; i++ ) {
	if ( ptr[i] == NULL ) {
	    *nospace = RETVAL_OK;
	    return i;
	}
    }
    *nospace = 1;
    return RETVAL_OK;
}

void
records_init() {
    int retval;

    if (initialized)
	return;

    hr_vector = (hostrecord_t**)malloc( sizeof(hostrecord_t*) * hr_vector_size);
    if ( hr_vector == NULL ) {
	out_err("Unable to locate memory for the host record vector. Dying now");
	exit (3);
    }
    memset ( hr_vector, 0, sizeof(hostrecord_t*) * hr_vector_size);

    retval = pthread_mutex_init(&vector_mutex, NULL);
    if ( retval != 0 ) {
	out_syserr(errno, "Error initializing vector_mutex");
	exit (3);
    }

    initialized = 1;
}

void
records_destroy () {
    unsigned long i;
    hostrecord_t **ptr;
    int retval;

    if (!initialized)
	return;

    ptr = hr_vector;
    for (i = 0; i < hr_vector_size; i++) {
	if ( ptr[i] != NULL )
	    free(ptr[i]);
    }
    free(ptr);

    retval = pthread_mutex_destroy ( &vector_mutex );
    if ( retval != 0 )
	out_syserr(errno, "Error destroying vector_mutex" );
}

int
records_maintenance() {
    int retval;
    hostrecord_t **ptr;
    unsigned long i, last_used, new_size;

    if (!initialized)
	return RETVAL_ERR;

    retval = pthread_mutex_lock(&vector_mutex);
    if ( retval != 0 ) {
	out_syserr(errno, "Unable to lock vector_mutex");
	return RETVAL_ERR;
    }

    out_dbg("Maintenance: Before maintenance: vector size %li, fill %li, chunk_size %i", hr_vector_size, hr_vector_fill, hr_vector_chunksize);

    ptr = hr_vector;
    for (i=0; i<hr_vector_size; i++) {
	if (ptr[i] != NULL &&
	    ptr[i]->remove != 0 ) {
	    out_dbg("Maintenance: remove entry %li due to request (%i)",
		    i,
		    ptr[i]->remove);
	    free(ptr[i]);
	    ptr[i] = NULL;
	}
    }

    /* find the last used slot */
    last_used = 0;
    ptr = hr_vector;
    for (i=0; i<hr_vector_size; i++) {
	if ( ptr[i] != NULL )
	    last_used = i + 1;
    }
    out_dbg("Maintenance: Last used slot %li",
	    last_used);

    /* If everything is used, bail out */
    if ( last_used == hr_vector_size )
	goto END_OK;

    new_size =
	ceil((double)last_used / (double)hr_vector_chunksize) *
	hr_vector_chunksize;
    out_dbg("Maintenance: computed new size %li",
	    new_size);
    assert ( (new_size % hr_vector_chunksize) == 0 );

    new_size = new_size < hr_vector_chunksize ? hr_vector_chunksize : new_size;
    out_dbg("Maintenance: computed new size used %li",
	    new_size);
    assert ( new_size >= last_used );

    hr_vector_size = new_size;
    hr_vector_fill = last_used;

    out_dbg("Maintenance: address of vector before realloc() %p",
	    hr_vector);
    hr_vector = (hostrecord_t**)realloc(hr_vector, sizeof(hostrecord_t*) * new_size);
    if ( hr_vector == NULL ) {
	out_err("Unable to shrink hr_vector");
	exit (3);
    }
    out_dbg("Maintenance: address of vector after realloc() %p",
	    hr_vector);
    out_dbg("Maintenance: After maintenance: vector size %li, fill %li, chunk_size %i", hr_vector_size, hr_vector_fill, hr_vector_chunksize);
    goto END_OK;

 END_OK:
    retval = pthread_mutex_unlock(&vector_mutex);
    if ( retval != 0 ) {
	out_syserr(errno, "Unable to unlock vector_mutex");
	return RETVAL_ERR;
    }
    return RETVAL_OK;
 /* END_ERR: */
 /*    retval = pthread_mutex_unlock(&vector_mutex); */
 /*    if ( retval != 0 ) { */
 /* 	out_syserr(errno, "Unable to unlock vector_mutex"); */
 /* 	return RETVAL_ERR; */
 /*    } */
 /*    return RETVAL_ERR; */
}

int
records_add(const char* ipaddr) {
    int retval;
    unsigned long pos_newrec;
    hostrecord_t *ptr;
    config *cfg;

    if (ipaddr == NULL)
	return RETVAL_ERR;

    if (!initialized)
	return RETVAL_ERR;

    retval = pthread_mutex_lock(&vector_mutex);
    if ( retval != 0 ) {
	out_syserr(errno, "Unable to lock vector_mutex");
	return RETVAL_ERR;
    }

    ptr = _records_find(ipaddr, NULL);
    if ( ptr != NULL ) {
	cfg = config_get();
	if ( cfg == NULL ) {
	    /* If we run with make check, the code below must not be executed */
#ifndef CHECK
	    out_err("<NULL> configuration received while adding record!.");
	    goto END_ERR;
#else
#warning "CHECK is enabled"
#endif
	}

	ptr->lastseen = time (NULL);

	/* Bump the occurrence only if within configured time interval */
	/* If we run with make check, the code below must not be executed */
#ifndef CHECK
	if ( ptr->lastseen - ptr->firstseen <= cfg->time_interval )
	    ptr->occurrences++;
#else
#warning "CHECK is enabled"
#endif

	goto END_OK;
    }

    /*
     * Add the new record
     */
    pos_newrec = _records_find_free(&retval);
    if ( pos_newrec == 0 && retval != RETVAL_OK ) {
	pos_newrec = _records_vector_grow();
    }
    hr_vector[pos_newrec] = (hostrecord_t*) malloc ( sizeof(hostrecord_t) );
    if (hr_vector[pos_newrec] == NULL) {
	out_err("Unable to allocate space for hostrecord");
	goto END_ERR;
    }
    strncpy(hr_vector[pos_newrec]->ipaddr, ipaddr, IPADDR_SIZE);
    hr_vector[pos_newrec]->firstseen = time(NULL);
    hr_vector[pos_newrec]->lastseen = 0;
    hr_vector[pos_newrec]->occurrences = 1;
    hr_vector[pos_newrec]->remove = 0;
    hr_vector[pos_newrec]->processed = 0;

    /* Make sure we update the fill value */
    hr_vector_fill =  (pos_newrec+1) > hr_vector_fill ? (pos_newrec+1) : hr_vector_fill;

    goto END_OK;

 END_OK:
    retval = pthread_mutex_unlock(&vector_mutex);
    if ( retval != 0 ) {
	out_syserr(errno, "Unable to unlock vector_mutex");
	return RETVAL_ERR;
    }
    return RETVAL_OK;
 END_ERR:
    retval = pthread_mutex_unlock(&vector_mutex);
    if ( retval != 0 ) {
	out_syserr(errno, "Unable to unlock vector_mutex");
	return RETVAL_ERR;
    }
    return RETVAL_ERR;
}

int
records_remove(const char *ipaddr) {
    int retval;
    unsigned long pos;
    hostrecord_t *ptr;

    if (ipaddr == NULL)
	return RETVAL_ERR;

    if (!initialized)
	return RETVAL_ERR;

    retval = pthread_mutex_lock(&vector_mutex);
    if ( retval != 0 ) {
	out_syserr(errno, "Unable to lock vector_mutex");
	return RETVAL_ERR;
    }

    ptr = _records_find(ipaddr, &pos);
    if (ptr != NULL) {
	free(ptr);
	hr_vector[pos] = NULL;
    } else {
	goto END_ERR;
    }

    retval = pthread_mutex_unlock(&vector_mutex);
    if ( retval != 0 ) {
	out_syserr(errno, "Unable to unlock vector_mutex");
	return RETVAL_ERR;
    }
    return RETVAL_OK;

 END_ERR:
    retval = pthread_mutex_unlock(&vector_mutex);
    if ( retval != 0 ) {
	out_syserr(errno, "Unable to unlock vector_mutex");
	return RETVAL_ERR;
    }
    return RETVAL_ERR;
}

int
records_enumerate(records_enum_callback cb) {
    int retval;
    hostrecord_t **ptr;
    unsigned long i;

    if (cb == NULL)
	return RETVAL_ERR;

    if (!initialized)
	return RETVAL_ERR;

    retval = pthread_mutex_lock(&vector_mutex);
    if ( retval != 0 ) {
	out_syserr(errno, "Unable to lock vector_mutex");
	return RETVAL_ERR;
    }

    ptr = hr_vector;
    for (i = 0; i < hr_vector_fill; i++ ) {
	if ( ptr[i] != NULL ) {
	    retval = cb(ptr[i]);
	    if (retval != 0)
		break;
	}
    }

    goto END_OK;

 END_OK:
    retval = pthread_mutex_unlock(&vector_mutex);
    if ( retval != 0 ) {
	out_syserr(errno, "Unable to unlock vector_mutex");
	return RETVAL_ERR;
    }
    return RETVAL_OK;
 /* END_ERR: */
 /*    retval = pthread_mutex_unlock(&vector_mutex); */
 /*    if ( retval != 0 ) { */
 /* 	out_syserr(errno, "Unable to unlock vector_mutex"); */
 /* 	return RETVAL_ERR; */
 /*    } */
 /*    return RETVAL_ERR; */
}

hostrecord_t
*records_get(const char *ipaddr) {
    int retval;
    hostrecord_t *ptr;

    if (ipaddr == NULL)
	return NULL;

    if (!initialized)
	return NULL;

    retval = pthread_mutex_lock(&vector_mutex);
    if ( retval != 0 ) {
	out_syserr(errno, "Unable to lock vector_mutex");
	return NULL;
    }

    ptr = _records_find(ipaddr, NULL);

    goto END_OK;

 END_OK:
    retval = pthread_mutex_unlock(&vector_mutex);
    if ( retval != 0 ) {
	out_syserr(errno, "Unable to unlock vector_mutex");
	return NULL;
    }
    return ptr;
 /* END_ERR: */
 /*    retval = pthread_mutex_unlock(&vector_mutex); */
 /*    if ( retval != 0 ) { */
 /* 	out_syserr(errno, "Unable to unlock vector_mutex"); */
 /* 	return NULL; */
 /*    } */
 /*    return NULL; */
}
