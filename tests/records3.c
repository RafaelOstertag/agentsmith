
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

#define BUFFSIZE 256
#define ITERATIONS 5

int
main(int wdc1, char **wdc2) {
    int       retval;
    hostrecord_t *ptr;
    char      str[BUFFSIZE];
    unsigned long i, k;

    records_init();

    for (i = 0; i < ITERATIONS; i++) {
	for (k = 0; k < records_dbg_get_vector_chunksize(); k++) {
	    snprintf(str, BUFFSIZE, "192.168.%i.%i", i, k);
	    retval = records_add_ip(str);
	    if (retval != 0) {
		fprintf(stderr, "records_add_ip() returned %i\n", retval);
		exit(1);
	    }
	}
	for (k = 0; k < records_dbg_get_vector_chunksize(); k++) {
	    snprintf(str, BUFFSIZE, "192.168.%i.%i", i, k);
	    retval = records_remove(str);
	    if (retval != 0) {
		fprintf(stderr, "records_remove() returned %i\n", retval);
		exit(1);
	    }
	}
    }

    for (i = 0; i < ITERATIONS; i++) {
	for (k = 0; k < records_dbg_get_vector_chunksize(); k++) {
	    snprintf(str, BUFFSIZE, "192.168.%i.%i", i, k);
	    ptr = records_get(str);
	    if (ptr != NULL) {
		fprintf(stderr,
			"records_get() for '%s' was successful which shouldn't happen.\n",
			str);
		exit(1);
	    }
	}
    }

    /*
     * Since we kept within the chunk size, the vector size may not be greater
     * than chunk_size 
     */
    if (records_dbg_get_vector_size() != records_dbg_get_vector_chunksize()) {
	fprintf(stderr, "Vector grew unexpected.\n");
	exit(1);
    }
    records_destroy(NULL);
    exit(0);
}
