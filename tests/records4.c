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

    /* now remove the records again */
    for (i=0; i<ITERATIONS; i++) {
	for (k=0; k<ITERATIONS; k++) {
	    snprintf(str, BUFFSIZE, "192.168.%i.%i", i, k);
	    retval = records_remove(str);
	    if (retval != 0) {
		fprintf(stderr, "records_remove() returned %i for '%s'\n", retval, str);
		exit(1);
	    }
	}
    }

    /* fill in records, but make sure ITERATIONS is below
       records_dbg_vector_chunksize()
     */
    i=0;
    for (k=0; k<ITERATIONS; k++) {
	snprintf(str, BUFFSIZE, "192.168.%i.%i", i, k);

	retval = records_add(str);
	if (retval != 0) {
	    fprintf(stderr, "records_add() returned %i\n", retval);
	    exit(1);
	}
    }

    /* Now perform the maintenance */
    retval = records_maintenance(NULL);
    if (retval != 0) {
	out_err("Error in records_maintenance()\n");
	exit (1);
    }

    /* the vector size must now be equal the chunksize */
    if (records_dbg_get_vector_size() != records_dbg_get_vector_chunksize()) {
	out_err("The vector size is not equal the chunk size.\n");
	exit (1);
    }
    records_destroy(NULL);
    exit (0);
}
