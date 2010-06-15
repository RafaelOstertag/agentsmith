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
#define EXPECTED_FILL 3

/*
 * Check the maintenance function to make sure we loose nothing
 */

int main (int wdc1, char** wdc2) {
    int retval;
    char str[BUFFSIZE];
    unsigned long i,k;

    records_init();

    retval = records_add("192.168.100.1");
    if (retval != 0) {
	fprintf(stderr, "records_add() returned %i\n", retval);
	exit(1);
    }

    retval = records_add("192.168.100.2");
    if (retval != 0) {
	fprintf(stderr, "records_add() returned %i\n", retval);
	exit(1);
    }

    retval = records_add("192.168.100.3");
    if (retval != 0) {
	fprintf(stderr, "records_add() returned %i\n", retval);
	exit(1);
    }

    retval = records_remove("192.168.100.1");
    if (retval != 0) {
	fprintf(stderr, "records_remove() returned %i\n", retval);
	exit(1);
    }

    retval = records_add("192.168.100.2");
    if (retval != 0) {
	fprintf(stderr, "records_add() returned %i\n", retval);
	exit(1);
    }

    if ( records_dbg_get_vector_fill() != EXPECTED_FILL ) {
	fprintf(stderr, "Expected fill is %i, but the actual value %il differs.\n", EXPECTED_FILL, records_dbg_get_vector_fill() );
    }

    retval = records_maintenance(NULL);
    if (retval != 0) {
	fprintf(stderr, "records_maintenance() returned %i\n", retval);
	exit(1);
    }

    if ( records_dbg_get_vector_fill() != EXPECTED_FILL ) {
	fprintf(stderr, "After maintenance, expected fill is %i, but the actual value %il differs.\n", EXPECTED_FILL, records_dbg_get_vector_fill() );
    }

    records_destroy(NULL);
    exit (0);
}
