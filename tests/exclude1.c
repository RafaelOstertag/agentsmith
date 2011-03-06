
/* $Id$
 *
 * Tests the exclude list functionality
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

#include "exclude.h"

#define BUFFSIZE 256

int
main(int wdc1, char **wdc2) {
    int       retval, a, b;
    char      buff[BUFFSIZE];

    exclude_init();

    /*
     * This is mainly used for checking for leaks using dbx 
     */
    for (a = 1; a < 10; a++)
	for (b = 1; b < 10; b++) {
	    snprintf(buff, BUFFSIZE, "%d.%d.0.0/24", a, b);
	    retval = exclude_add(buff);
	    if (retval != 0)
		exit(1);
	}

    retval = exclude_clear();
    if (retval != 0)
	exit(1);

    /*
     * Make sure the list has been cleared 
     */
    for (a = 1; a < 10; a++)
	for (b = 1; b < 10; b++) {
	    snprintf(buff, BUFFSIZE, "%d.%d.1.1", a, b);
	    retval = exclude_isexcluded(buff);
	    if (retval == 0)
		exit(1);
	}

    /*
     * Add IPs 
     */

    retval = exclude_add("192.168.240.21");
    if (retval != 0)
	exit(1);

    retval = exclude_add("192.1.1.0/24");
    if (retval != 0)
	exit(1);

    retval = exclude_add("10.1.0.0/20");
    if (retval != 0)
	exit(1);

#ifdef HAVE_IN6_ADDR_T
    retval = exclude_add("2001:1:2:e::/61");
    if (retval != 0)
	exit(1);
#else
#warning "Not testing IPv6"
#endif

    /*
     * Check for exclusion 
     */

    retval = exclude_isexcluded("192.1.1.0");
    if (retval != 0)
	exit(1);

    retval = exclude_isexcluded("192.1.1.1");
    if (retval != 0)
	exit(1);

    retval = exclude_isexcluded("192.1.1.128");
    if (retval != 0)
	exit(1);

    retval = exclude_isexcluded("192.1.1.255");
    if (retval != 0)
	exit(1);

    retval = exclude_isexcluded("192.1.2.1");
    if (retval == 0)
	exit(1);

    retval = exclude_isexcluded("10.1.0.0");
    if (retval != 0)
	exit(1);

    retval = exclude_isexcluded("10.1.15.255");
    if (retval != 0)
	exit(1);

    retval = exclude_isexcluded("192.168.240.21");
    if (retval != 0)
	exit(1);

#ifdef HAVE_IN6_ADDR_T
    retval = exclude_isexcluded("2001:1:2:8::");
    if (retval != 0)
	exit(1);

    retval = exclude_isexcluded("2001:1:2:8::0");
    if (retval != 0)
	exit(1);

    retval = exclude_isexcluded("2001:1:2:f:ffff:ffff:ffff:ffff");
    if (retval != 0)
	exit(1);

    retval = exclude_isexcluded("2001:1:2:f:ffff:ffff:ffff:1");
    if (retval != 0)
	exit(1);
#else
#warning "Not testing IPv6"
#endif

    /*
     * IPs which may not be excluded 
     */

    retval = exclude_isexcluded("192.168.240.22");
    if (retval == 0)
	exit(1);

    retval = exclude_isexcluded("192.168.240.20");
    if (retval == 0)
	exit(1);

    retval = exclude_isexcluded("10.1.16.1");
    if (retval == 0)
	exit(1);

    retval = exclude_isexcluded("10.0.0.1");
    if (retval == 0)
	exit(1);

#ifdef HAVE_IN6_ADDR_T
    retval = exclude_isexcluded("2001:1:2:7::1");
    if (retval == 0)
	exit(1);
#else
#warning "Not testing IPv6"
#endif

    exclude_destroy();

    exit(0);
}
