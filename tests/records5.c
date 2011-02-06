/* $Id$
 *
 * Tests the records functions single threaded.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#include "records.h"
#include "output.h"

#define BUFFSIZE 256
#define ITERATIONS 32

unsigned long num_entries = 0;

int
callback1(hostrecord_t *ptr) {
    num_entries++;
    return 0;
}

int
callback2(hostrecord_t *ptr) {
    ptr->remove = 1;
    return 0;
}

int main (int wdc1, char** wdc2) {
    int retval;
    hostrecord_t *ptr;
    char str[BUFFSIZE];
    unsigned long i,k;

    records_init();

    /* Fill the records in. It is expected that some reallocations take place,
       so make surce ITERATIONS * ITERATIONS > records_dbg_get_chunksize().
    */
    for (i=0; i<ITERATIONS; i++) {
	for (k=0; k<ITERATIONS; k++) {
	    snprintf(str, BUFFSIZE, "192.168.%i.%i", i, k);
	    retval = records_add(str);
	    if (retval != 0) {
		fprintf(stderr, "records_add() returned %i\n", retval);
		exit(1);
	    }
	}
    }

    retval = records_enumerate(callback1, SYNC);
    if (retval != 0) {
	out_err("records_enumerate() returned value %i", retval);
	exit(1);
    }

    if (num_entries != ITERATIONS*ITERATIONS) {
	out_err("records_enumerate() did not enumerate all records %i, but only %i",
		ITERATIONS*ITERATIONS,num_entries);
	exit(1);
    }

    /* 
     * here we test the hostrecord_t.remove field 
     */

    for (i=0; i<ITERATIONS; i++) {
	for (k=0; k<ITERATIONS; k++) {
	    snprintf(str, BUFFSIZE, "192.168.%i.%i", i, k);
	    retval = records_add(str);
	    if (retval != 0) {
		fprintf(stderr, "records_add() returned %i\n", retval);
		exit(1);
	    }
	}
    }

    retval = records_enumerate(callback2, SYNC);
    if (retval != 0) {
	out_err("records_enumerate() returned value %i", retval);
	exit(1);
    }

    retval = records_maintenance(NULL);
    if (retval != 0) {
	out_err("records_maintenance() returned value %i", retval);
	exit(1);
    }

    /* Make absolutely positively sure there is no leftover */
    for (i=0; i<ITERATIONS; i++) {
	for (k=0; k<ITERATIONS; k++) {
	    hostrecord_t *ptr;
	    snprintf(str, BUFFSIZE, "192.168.%i.%i", i, k);
	    ptr = records_get(str);
	    if (ptr != 0) {
		fprintf(stderr, "records_get() unexpected returned a record for '%s'\n", ptr->ipaddr);
		exit(1);
	    }
	}
    }

    if ( records_dbg_get_vector_size() != records_dbg_get_vector_chunksize() ) {
	out_err("records_dbg_get_vector_size() != records_dbg_get_vector_chunksize().\nrecords_dbg_get_vector_size() == %i\nrecords_dbg_get_vector_chunksize()==%i",records_dbg_get_vector_size(),records_dbg_get_vector_chunksize());
	exit(1);
    }
    if ( records_dbg_get_vector_fill() != 0 ) {
	out_err("records_dbg_get_vector_fill().\nrecords_dbg_get_vector_fill()==%i.",records_dbg_get_vector_fill());
	exit(1);
    }
    records_destroy(NULL);
    exit (0);
}
