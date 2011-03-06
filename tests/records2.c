
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
#define ITERATIONS 32

int
main(int wdc1, char **wdc2) {
    int       retval;
    hostrecord_t *ptr;
    char      str[BUFFSIZE];
    unsigned long i, k;

    records_init();

    for (i = 0; i < ITERATIONS; i++) {
	for (k = 0; k < ITERATIONS; k++) {
	    snprintf(str, BUFFSIZE, "192.168.%i.%i", i, k);
	    retval = records_add_ip(str);
	    if (retval != 0) {
		fprintf(stderr, "records_add() returned %i\n", retval);
		exit(1);
	    }
	}
    }
    for (i = 0; i < ITERATIONS; i++) {
	for (k = 0; k < ITERATIONS; k++) {
	    snprintf(str, BUFFSIZE, "192.168.%i.%i", i, k);
	    ptr = records_get(str);
	    if (ptr == NULL) {
		fprintf(stderr, "records_get() for '%s' was unsuccessful.\n",
			str);
		exit(1);
	    }
	}
    }
    records_destroy(NULL);
    exit(0);
}
