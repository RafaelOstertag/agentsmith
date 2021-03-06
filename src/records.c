
/* Copyright (C) 2010 Rafael Ostertag
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

#include "globals.h"
#include "cfg.h"
#include "output.h"
#include "records.h"

struct _records_enumerate_arguments {
    records_enum_callback cb;
};
typedef struct _records_enumerate_arguments records_enumerate_arguments;

static hostrecord_t **hr_vector = NULL;

/* The chunk size we pre-allocate */
static unsigned long hr_vector_chunksize = 100;

/* This will be dynamically expanded as needed */
static unsigned long hr_vector_size = 100;
static unsigned long hr_vector_fill = 0;

pthread_mutex_t vector_mutex;

static int initialized = 0;

unsigned long
records_dbg_get_vector_size() {
    return hr_vector_size;
}

unsigned long
records_dbg_get_vector_chunksize() {
    return hr_vector_chunksize;
}

unsigned long
records_dbg_get_vector_fill() {
    return hr_vector_fill;
}

/**
 * This is function doing the actual listing of records.
 */
static int
_records_enumerate(records_enum_callback cb) {
    hostrecord_t **ptr;
    int       retval;
    unsigned long i;

    assert(cb != NULL);

    retval = pthread_mutex_lock(&vector_mutex);
    if (retval != 0) {
	out_syserr(retval, "Unable to lock vector_mutex");
	return RETVAL_ERR;
    }

    ptr = hr_vector;
    for (i = 0; i < hr_vector_fill; i++) {
	if (ptr[i] != NULL) {
	    retval = cb(ptr[i]);
	    if (retval != 0)
		break;
	}
    }

    retval = pthread_mutex_unlock(&vector_mutex);
    if (retval != 0) {
	out_syserr(retval, "Unable to unlock vector_mutex");
	return RETVAL_ERR;
    }

    return RETVAL_OK;
}

/**
 * This is thread function for doing the record enumeration asynchronously.
 *
 * @param args a pointer to the call back function.
 *
 */
static void *
_records_enumerate_thread(void *args) {
    int       retval;
    records_enumerate_arguments *rec_args;
    records_enum_callback cb;

    assert(args != NULL);
    
    rec_args = (records_enumerate_arguments*) args;

    cb = (records_enum_callback) rec_args->cb;
    
    free(args);

    retval = _records_enumerate(cb);
    if (retval != RETVAL_OK)
	out_err("Error enumerating records asynchronously");

    pthread_exit(NULL);
}

/**
 * @return
 * The index of the first free slot in the newly allocated vector.
 */
static unsigned long
_records_vector_grow() {
    unsigned long previous_size;

    previous_size = hr_vector_size;
    hr_vector_size += hr_vector_chunksize;

    hr_vector = realloc((void *) hr_vector,
			sizeof (hostrecord_t **) * hr_vector_size);
    if (hr_vector == NULL) {
	out_err("Unable to allocate more space for hr_vector. Fatal");
	exit(3);
    }
    memset(hr_vector + previous_size, 0,
	   sizeof (hostrecord_t **) * hr_vector_chunksize);

    return previous_size;
}

static hostrecord_t *
_records_find(const char *ipaddr, unsigned long *pos) {
    hostrecord_t **ptr;
    unsigned long i;

    ptr = hr_vector;
    for (i = 0; i < hr_vector_fill; i++) {
	if (ptr[i] != NULL) {
	    if (strcmp(ptr[i]->ipaddr, ipaddr) == 0) {
		if (pos != NULL)
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

    assert(nospace != NULL);

    ptr = hr_vector;
    for (i = 0; i < hr_vector_size; i++) {
	if (ptr[i] == NULL) {
	    *nospace = RETVAL_OK;
	    return i;
	}
    }
    *nospace = 1;
    return RETVAL_OK;
}

void
records_init() {
    int       retval;

    assert(initialized == 0);

    hr_vector =
	(hostrecord_t **) calloc(sizeof (hostrecord_t *), hr_vector_size);
    if (hr_vector == NULL) {
	out_err
	    ("Unable to allocate memory for the host record vector. Dying now");
	exit(3);
    }
    memset(hr_vector, 0, sizeof (hostrecord_t *) * hr_vector_size);

    retval = pthread_mutex_init(&vector_mutex, NULL);
    if (retval != 0) {
	out_syserr(retval, "Error initializing vector_mutex");
	exit(3);
    }

    initialized = 1;
}

void
records_destroy(records_remove_callback cb) {
    unsigned long i;
    hostrecord_t **ptr;
    int       retval;

    assert(initialized);

    ptr = hr_vector;
    for (i = 0; i < hr_vector_size; i++) {
	if (ptr[i] != NULL) {
	    if (cb != NULL)
		cb(ptr[i]);
	    free(ptr[i]);
	}
    }
    free(ptr);

    retval = pthread_mutex_destroy(&vector_mutex);
    if (retval != 0)
	out_syserr(retval, "Error destroying vector_mutex");
}

/*
 * The callback function may be NULL.
 */
int
records_maintenance(records_remove_callback cb) {
    int       retval;
    hostrecord_t **ptr;
    unsigned long i, last_used, new_size;

    assert(initialized);

    retval = pthread_mutex_lock(&vector_mutex);
    if (retval != 0) {
	out_syserr(retval, "Unable to lock vector_mutex");
	return RETVAL_ERR;
    }

    out_dbg
	("Maintenance: Before maintenance: vector size %li, fill %li, chunk_size %i",
	 hr_vector_size, hr_vector_fill, hr_vector_chunksize);

    ptr = hr_vector;
    for (i = 0; i < hr_vector_size; i++) {
	if (ptr[i] != NULL && ptr[i]->remove != 0) {
	    if (cb != NULL) {
		out_dbg("Maintenance: call callback function");
		cb(ptr[i]);
	    }
	    out_dbg("Maintenance: remove entry %li due to request (%i)",
		    i, ptr[i]->remove);
	    free(ptr[i]);
	    ptr[i] = NULL;
	}
    }

    /*
     * find the last used slot 
     */
    last_used = 0;
    ptr = hr_vector;
    for (i = 0; i < hr_vector_size; i++) {
	if (ptr[i] != NULL)
	    last_used = i + 1;
    }
    out_dbg("Maintenance: Last used slot %li", last_used);

    /*
     * If everything is used, bail out 
     */
    if (last_used == hr_vector_size)
	goto END_OK;

    new_size =
	ceil((double) last_used / (double) hr_vector_chunksize) *
	hr_vector_chunksize;
    out_dbg("Maintenance: computed new size %li", new_size);
    assert((new_size % hr_vector_chunksize) == 0);

    new_size =
	new_size < hr_vector_chunksize ? hr_vector_chunksize : new_size;
    out_dbg("Maintenance: computed new size used %li", new_size);
    assert(new_size >= last_used);

    hr_vector_size = new_size;
    hr_vector_fill = last_used;

    out_dbg("Maintenance: address of vector before realloc() %p", hr_vector);
    hr_vector =
	(hostrecord_t **) realloc(hr_vector,
				  sizeof (hostrecord_t *) * new_size);
    if (hr_vector == NULL) {
	out_err("Unable to shrink hr_vector");
	exit(3);
    }
    out_dbg("Maintenance: address of vector after realloc() %p", hr_vector);
    out_dbg
	("Maintenance: After maintenance: vector size %li, fill %li, chunk_size %i",
	 hr_vector_size, hr_vector_fill, hr_vector_chunksize);
    goto END_OK;

  END_OK:
    retval = pthread_mutex_unlock(&vector_mutex);
    if (retval != 0) {
	out_syserr(retval, "Unable to unlock vector_mutex");
	return RETVAL_ERR;
    }
    return RETVAL_OK;
}

int
records_add_ip(const char *ipaddr) {
    int       retval;
    unsigned long pos_newrec;
    hostrecord_t *ptr;

    assert(initialized);
    assert(ipaddr != NULL);

    retval = pthread_mutex_lock(&vector_mutex);
    if (retval != 0) {
	out_syserr(retval, "Unable to lock vector_mutex");
	return RETVAL_ERR;
    }

    ptr = _records_find(ipaddr, NULL);
    if (ptr != NULL) {
	ptr->lastseen = time(NULL);

	/*
	 * Bump the occurrence only if within configured time interval 
	 */
	/*
	 * If we run with make check, the code below must not be executed 
	 */
#ifndef CHECK
	if (ptr->lastseen - ptr->firstseen <= ptr->time_interval)
	    ptr->occurrences++;
#endif

	goto END_OK;
    }

    /*
     * Add the new record
     */
    pos_newrec = _records_find_free(&retval);
    if (pos_newrec == 0 && retval != RETVAL_OK) {
	pos_newrec = _records_vector_grow();
    }
    hr_vector[pos_newrec] = (hostrecord_t *) malloc(sizeof (hostrecord_t));
    if (hr_vector[pos_newrec] == NULL) {
	out_err("Unable to allocate space for hostrecord");
	goto END_ERR;
    }

    /*
     * If this record is sent to another agentsmith, the sender has to make
     * sure the origin is updated properly 
     */
    strncpy(hr_vector[pos_newrec]->origin, LOCALHOST, IPADDR_SIZE);
    hr_vector[pos_newrec]->origin[IPADDR_SIZE - 1] = '\0';

    strncpy(hr_vector[pos_newrec]->ipaddr, ipaddr, IPADDR_SIZE);
    hr_vector[pos_newrec]->ipaddr[IPADDR_SIZE - 1] = '\0';

    hr_vector[pos_newrec]->firstseen = time(NULL);
    hr_vector[pos_newrec]->lastseen = 0;
#ifndef CHECK
    hr_vector[pos_newrec]->time_interval = CONFIG.time_interval;
    hr_vector[pos_newrec]->purge_after = CONFIG.purge_after;
    hr_vector[pos_newrec]->action_threshold = CONFIG.action_threshold;
#endif
    hr_vector[pos_newrec]->occurrences = 1;
    hr_vector[pos_newrec]->remove = 0;
    hr_vector[pos_newrec]->processed = 0;

    /*
     * Make sure we update the fill value 
     */
    hr_vector_fill =
	(pos_newrec + 1) > hr_vector_fill ? (pos_newrec + 1) : hr_vector_fill;

    goto END_OK;

  END_OK:
    retval = pthread_mutex_unlock(&vector_mutex);
    if (retval != 0) {
	out_syserr(retval, "Unable to unlock vector_mutex");
	return RETVAL_ERR;
    }
    return RETVAL_OK;
  END_ERR:
    retval = pthread_mutex_unlock(&vector_mutex);
    if (retval != 0) {
	out_syserr(retval, "Unable to unlock vector_mutex");
	return RETVAL_ERR;
    }
    return RETVAL_ERR;
}

/**
 * This function adds an hostrecord verbatim to the vector.
 */
int
records_add_record(const hostrecord_t *hr) {
    int       retval;
    unsigned long pos_newrec;
    hostrecord_t *ptr;

    assert(hr != NULL);
    assert(initialized);

    retval = pthread_mutex_lock(&vector_mutex);
    if (retval != 0) {
	out_syserr(retval, "Unable to lock vector_mutex");
	return RETVAL_ERR;
    }

    /*
     * Check if we already have a record of this ip 
     */
    ptr = _records_find(hr->ipaddr, NULL);
    if (ptr != NULL) {
	uint32_t  processed, occurrences;
	/*
	 * Save the processed member of the existing record 
	 */
	processed = ptr->processed;
	/*
	 * Save the occurrences member of the existing record 
	 */
	occurrences = ptr->occurrences;
	/*
	 * Now copy the record over 
	 */
	memcpy(ptr, hr, sizeof (hostrecord_t));
	/*
	 * Adjust the processed and occurrences member 
	 */
	ptr->processed = processed;
	ptr->occurrences += occurrences;

	ptr->lastseen = time(NULL);

	if (ptr->lastseen - ptr->firstseen <= ptr->time_interval)
	    ptr->occurrences++;

	goto END_OK;
    }

    /*
     * Add the new record
     */
    pos_newrec = _records_find_free(&retval);
    if (pos_newrec == 0 && retval != RETVAL_OK) {
	pos_newrec = _records_vector_grow();
    }
    hr_vector[pos_newrec] = (hostrecord_t *) malloc(sizeof (hostrecord_t));
    if (hr_vector[pos_newrec] == NULL) {
	out_err("Unable to allocate space for hostrecord");
	goto END_ERR;
    }

    memcpy(hr_vector[pos_newrec], hr, sizeof (hostrecord_t));

    /*
     * update the fill value of the vector 
     */
    hr_vector_fill =
	(pos_newrec + 1) > hr_vector_fill ? (pos_newrec + 1) : hr_vector_fill;

    goto END_OK;

  END_OK:
    retval = pthread_mutex_unlock(&vector_mutex);
    if (retval != 0) {
	out_syserr(retval, "Unable to unlock vector_mutex");
	return RETVAL_ERR;
    }
    return RETVAL_OK;
  END_ERR:
    retval = pthread_mutex_unlock(&vector_mutex);
    if (retval != 0) {
	out_syserr(retval, "Unable to unlock vector_mutex");
	return RETVAL_ERR;
    }
    return RETVAL_ERR;
}

int
records_remove(const char *ipaddr) {
    int       retval;
    unsigned long pos;
    hostrecord_t *ptr;

    assert(ipaddr != NULL);
    assert(initialized);

    retval = pthread_mutex_lock(&vector_mutex);
    if (retval != 0) {
	out_syserr(retval, "Unable to lock vector_mutex");
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
    if (retval != 0) {
	out_syserr(retval, "Unable to unlock vector_mutex");
	return RETVAL_ERR;
    }
    return RETVAL_OK;

  END_ERR:
    retval = pthread_mutex_unlock(&vector_mutex);
    if (retval != 0) {
	out_syserr(retval, "Unable to unlock vector_mutex");
	return RETVAL_ERR;
    }
    return RETVAL_ERR;
}

/**
 * Enumerates the records and calls a function for each record. Depending on
 * the return value of the call-back function, the enumeration may stop.
 *
 * The actual enumeration is done in a separate detached thread to avoid race
 * conditions when called from a signal handler.
 *
 * @param cb the address of the function to call.
 *
 * @return 0 on success, -1 otherwise.
 */
int
records_enumerate(records_enum_callback cb, enum_mode_t mode) {
    int       retval;
    pthread_attr_t tattr;
    pthread_t wdc;
    records_enumerate_arguments *thread_args;

    assert(cb != NULL);
    assert(initialized);

    if (mode == ASYNC) {
	retval = pthread_attr_init(&tattr);
	if (retval != 0) {
	    out_syserr(retval,
		       "Error initializing thread attributes for record enumeration thread");
	    return RETVAL_ERR;
	}

	retval = pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
	if (retval != 0) {
	    out_syserr(retval,
		       "Error setting detach state for record enumeration thread");
	    return RETVAL_ERR;
	}
        
        // The thread will free the memory occupied.
        thread_args = malloc(sizeof(records_enumerate_arguments));
        if (thread_args == NULL) {
            out_err("No memory");
            return RETVAL_ERR;
        }
        
        thread_args->cb = cb;
        
	retval =
	    pthread_create(&wdc, &tattr, _records_enumerate_thread,
			   (void *) thread_args);
	if (retval != 0) {
	    out_syserr(retval, "Error launching record enumeration thread");
	    return RETVAL_ERR;
	}

	return RETVAL_OK;
    } else {
	return _records_enumerate(cb);
    }
}

hostrecord_t *
records_get(const char *ipaddr) {
    int       retval;
    hostrecord_t *ptr;

    assert(ipaddr != NULL);
    assert(initialized);

    retval = pthread_mutex_lock(&vector_mutex);
    if (retval != 0) {
	out_syserr(retval, "Unable to lock vector_mutex");
	return NULL;
    }

    ptr = _records_find(ipaddr, NULL);

    goto END_OK;

  END_OK:
    retval = pthread_mutex_unlock(&vector_mutex);
    if (retval != 0) {
	out_syserr(retval, "Unable to unlock vector_mutex");
	return NULL;
    }
    return ptr;
}
