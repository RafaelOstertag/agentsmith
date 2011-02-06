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
 */

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "exclude.h"
#include "output.h"

static excluderecord_t **exclude_vector = NULL;
/* The chunk size we pre-allocate */
static unsigned long er_vector_chunksize = 100;
/* This will be dynamically expanded as needed */
static unsigned long er_vector_size = 100;
static unsigned long er_vector_fill = 0;
/* The max length of a line in the exclude file */
static const unsigned int max_exclude_line_length = 100;

static pthread_mutex_t exclude_mutex;

static int initialized = 0;

enum {
    RETVAL_OK = 0,
    RETVAL_ERR = -1
};

unsigned long exclude_dbg_get_vector_size() { return er_vector_size; }
unsigned long exclude_dbg_get_vector_chunksize() { return er_vector_chunksize; }
unsigned long exclude_dbg_get_vector_fill() { return er_vector_fill; }

#ifdef DEBUG
static void
_exclude_show_list() {
    int retval;
    excluderecord_t **ptr;
    char ip[46], mask[46];

    retval = pthread_mutex_lock(&exclude_mutex);
    if ( retval != 0 ) {
	out_syserr(errno, "Unable to lock exclude_mutex");
	return;
    }

    ptr = exclude_vector;
    for (int i = 0; i < er_vector_fill; i++ ) {
	if ( ptr[i] != NULL ) {
	    if ( ptr[i]->af == AF_INET ) {
		if ( inet_ntop(ptr[i]->af, &(ptr[i]->address.net), ip, 46) == NULL ) {
		    out_dbg("Error converting ip address in record #%d", i);
		}
		if ( inet_ntop(ptr[i]->af, &(ptr[i]->netmask.mask), mask, 46) == NULL ) {
		    out_dbg("Error converting  netmask in record #%d", i);
		}
	    } else {
#ifdef HAVE_IN6_ADDR_T
		if ( inet_ntop(ptr[i]->af, &(ptr[i]->address.net6), ip, 46) == NULL ) {
		    out_dbg("Error converting ip address in record #%d", i);
		}
		if ( inet_ntop(ptr[i]->af, &(ptr[i]->netmask.mask6), mask, 46) == NULL ) {
		    out_dbg("Error converting  netmask in record #%d", i);
		}
#endif
	    }
	    out_dbg("Exclude entry #%d: af %d, net %s, mask %s", i,ptr[i]->af,ip,mask);
	}
    }

    retval = pthread_mutex_unlock(&exclude_mutex);
    if ( retval != 0 ) {
	out_syserr(errno, "Unable to unlock exclude_mutex");
	return;
    }
}
#endif

/**
 * This is the thread reading an IP exclude file.
 *
 * @param args a char pointer to the file path of the file holding the excluded
 * ip addresses.
 */
static void*
_exclude_readfile_thread(void* args) {
    FILE* exfile;
    int retval, lineno=0;
    char *fp;
    char line[max_exclude_line_length];

    fp=(char*) args;

    if (fp == NULL || strlen(fp) == 0) {
	out_msg("No exclude file specified");
	pthread_exit(NULL);
    }

    retval = access(fp, F_OK | R_OK);
    if (retval != 0) {
	out_syserr(errno, "Unable to access exclude file %s", fp);
	out_msg("Exclude list not changed");
	pthread_exit(NULL);
    }
    out_dbg("Access ok to %s", fp);

    exfile = fopen(fp, "r");
    if (exfile == NULL) {
	out_syserr(errno, "Unable to open exclude file %s", fp);
	out_msg("Exclude list not changed");
	pthread_exit(NULL);
    }
    out_dbg("Open ok on %s", fp);

#ifdef DEBUG
    out_dbg("Exclude lists has following entries before clearing:");
    _exclude_show_list();
#endif

    retval = exclude_clear();
    if (retval != RETVAL_OK) {
	out_err("Unable to clear previous exclude list");
	fclose(exfile);
	pthread_exit(NULL);
    }
    out_dbg("Exclude list clear ok");

    while (fgets(line, max_exclude_line_length, exfile) != NULL) {
	lineno++;

	if (*line == '#') continue;

	/* Get rid of newline */
	if (strlen(line) > 1 &&
	    line[strlen(line)-1] == '\n')
	    line[strlen(line)-1] = '\0';


	out_dbg("Read exclude line: %s", line);

	retval = exclude_add(line);
	if (retval != RETVAL_OK) {
	    out_err("Error processing line %d in %s. Ignoring this line", lineno, fp);
	    continue;
	}
    }

#ifdef DEBUG
    out_dbg("Exclude lists has following entries after reading:");
    _exclude_show_list();
#endif

    fclose(exfile);
    out_dbg("Closed %s", fp);

    out_dbg("Exiting exclude read thread");
    pthread_exit(NULL);
}


/**
 * Returns the IP Address as string and the prefix as unsigned int.
 *
 * The returned IP Address string has to be free()'ed by the caller.
 */
static int
_exclude_parse_ip(const char* str, char** ip, unsigned int* prefix, int* af) {
    char* prefixptr;
    char *tmpstr;


    if ( ip == NULL ||
	 prefix == NULL ||
	 af == NULL ||
	 str == NULL ) return RETVAL_ERR;

    /* Check if str might be a valid ip address depending on its length */
    if (strlen(str) < 7) {
	out_err("%s is not a valid IP Address", str);
	return RETVAL_ERR;
    }

    /* Find out whether we have a prefix */
    prefixptr = strchr(str,'/');

    /* Find out whether it is an IPv4 or IPv6 address. We take that when ':'
       occurs in the string, it is an IPv6 address. */
    *af = strchr(str,':') != NULL ? AF_INET6 : AF_INET;

    *ip = (char*) malloc ( strlen(str)+1 );
    if (prefixptr) {
	strncpy(*ip, str, prefixptr-str);
	(*ip)[prefixptr-str]='\0';
	*prefix=(unsigned int) atoi(prefixptr+1);

	/*
	 * Do the sanity checks we can do so far
	 */
	if ( *prefix > 32 && *af == AF_INET ) {
	    out_err("%d is not valid prefix for %s", *prefix, *ip);
	    free(*ip);
	    *ip = NULL;
	    *af = 0;
	    *prefix = 0;
	    return RETVAL_ERR;
	}
#ifdef HAVE_IN6_ADDR_T
	if ( *prefix > 128 && *af == AF_INET6 ) {
	    out_err("%d is not valid prefix for %s", *prefix, *ip);
	    free(*ip);
	    *ip = NULL;
	    *af = 0;
	    *prefix = 0;
	    return RETVAL_ERR;
	}
#endif
    } else {
	strncpy(*ip, str, strlen(str) );

	/* get rid of newlines */
	if ( (*ip)[strlen(str)-1] == '\n' )
	    (*ip)[strlen(str)-1] = '\0' ;
	else
	    (*ip)[strlen(str)] = '\0' ;

	/* No prefix means single ip address, ie. a host address */
#ifdef HAVE_IN6_ADDR_T
	*prefix = *af == AF_INET6 ? 128 : 32;
#else
	*prefix = 32;
#endif
    }
    return RETVAL_OK;
}

#ifdef HAVE_IN6_ADDR_T
/**
 *
 */
static int
_exclude_prefix_to_ipv6_mask(unsigned int prefix, in6_addr_t *mask) {
    int n,m,i;

    assert(mask != NULL);
    assert(prefix >= 0 && prefix <= 128);

    memset(mask, 0, sizeof(*mask) );

    n = prefix / 8;
    for (i=0; i < n ; i++) {
#ifdef HAVE_SOLARIS_IN6_ADDR
	mask->_S6_un._S6_u8[i]=0xff;
#elif HAVE_BSD_IN6_ADDR
	mask->__u6_addr.__u6_addr8[i]=0xff;
#elif HAVE_LINUX_IN6_ADDR
	mask->__in6_u.__u6_addr8[i]=0xff;
#else
#error "Your in6_addr_t is not supported"
#endif
    }

    m = prefix % 8;
    if ( m == 0 )
	return RETVAL_OK;

#ifdef HAVE_SOLARIS_IN6_ADDR
    mask->_S6_un._S6_u8[n] = 0xff;
    mask->_S6_un._S6_u8[n] <<= 8-m;
#elif HAVE_BSD_IN6_ADDR
    mask->__u6_addr.__u6_addr8[n] = 0xff;
    mask->__u6_addr.__u6_addr8[n] <<= 8-m;
#elif HAVE_LINUX_IN6_ADDR
    mask->__in6_u.__u6_addr8[n] = 0xff;
    mask->__in6_u.__u6_addr8[n] <<= 8-m;
#endif

    return RETVAL_OK;
}
#endif

inline static int
_exclude_ipv4_in_net(in_addr_t net, in_addr_t mask, in_addr_t ip) {
    return ( (net & mask) == (ip & mask)) ? RETVAL_OK : RETVAL_ERR;
}

#ifdef HAVE_IN6_ADDR_T
inline static int
_exclude_ipv6_in_net(const in6_addr_t *net,
		     const in6_addr_t *mask,
		     const in6_addr_t *ip) {
    int i;

    assert(net != NULL && mask != NULL && ip != NULL);

    for (i=0; i<16; i++) {
	if (
#ifdef HAVE_SOLARIS_IN6_ADDR
	    (net->_S6_un._S6_u8[i] & mask->_S6_un._S6_u8[i]) !=
	    (ip->_S6_un._S6_u8[i] & mask->_S6_un._S6_u8[i])
#elif HAVE_BSD_IN6_ADDR
	    (net->__u6_addr.__u6_addr8[i] & mask->__u6_addr.__u6_addr8[i]) !=
	    (ip->__u6_addr.__u6_addr8[i] & mask->__u6_addr.__u6_addr8[i])
#elif HAVE_LINUX_IN6_ADDR
	    (net->__in6_u.__u6_addr8[i] & mask->__in6_u.__u6_addr8[i]) !=
	    (ip->__in6_u.__u6_addr8[i] & mask->__in6_u.__u6_addr8[i])
#else
#error "Your in6_addr_t is not supported"
#endif
	    )
	    return RETVAL_ERR;
    }
    return RETVAL_OK;
}
#endif

/**
 * @return
 * The index of the first free slot in the newly allocated vector.
 */
static unsigned long
_exclude_vector_grow () {
    unsigned long previous_size;

    previous_size = er_vector_size;
    er_vector_size += er_vector_chunksize;

    exclude_vector = realloc((void*) exclude_vector,
			sizeof(excluderecord_t**) * er_vector_size);
    if ( exclude_vector == NULL ) {
	out_err("Unable to allocate more space for hr_vector. Fatal");
	exit (3);
    }
    memset (exclude_vector + previous_size, 0, sizeof(excluderecord_t**) * er_vector_chunksize);

    return previous_size;
}

static excluderecord_t*
_records_find(const char* ipaddr, unsigned long *pos) {
    excluderecord_t **ptr;
    char *iponly = NULL;
    unsigned long i;
    unsigned int prefix;
    int af, retval;
    in_addr_t ipv4;
#ifdef HAVE_IN6_ADDR_T
    in6_addr_t ipv6;
#endif

    /* mainly used to get the address family */
    retval = _exclude_parse_ip(ipaddr, &iponly, &prefix, &af);
    if ( retval == RETVAL_ERR ) {
	out_err("Error parsing '%s' into components", ipaddr);
	*pos = 0;
	return NULL;
    }

#ifdef HAVE_IN6_ADDR_T
    assert( (af == AF_INET && prefix == 32) ||
	    ( af == AF_INET6 && prefix == 128 ) );
#else
    assert( af == AF_INET && prefix == 32 );
#endif

    if ( af == AF_INET ) {
	retval = inet_pton(af, iponly, &ipv4);
	if ( retval != 1 ) {
	    out_err("Error converting '%s' to numeric format", iponly);
	    goto ERR_END;
	}
    } else {
#ifdef HAVE_IN6_ADDR_T
	retval = inet_pton(af, iponly, &ipv6);
	if ( retval != 1 ) {
	    out_err("Error converting '%s' to numeric format", iponly);
	    goto ERR_END;
	}
#endif
    }

    ptr = exclude_vector;
    for (i = 0; i < er_vector_fill; i++ ) {
	if ( ptr[i] != NULL && ptr[i]->af == af) {
	    if ( ptr[i]->af == AF_INET ) {
		if ( RETVAL_ERR == _exclude_ipv4_in_net( ptr[i]->address.net,
							 ptr[i]->netmask.mask,
							 ipv4) )
		    continue;
	    } else {
#ifdef HAVE_IN6_ADDR_T
		if ( RETVAL_ERR ==
		     _exclude_ipv6_in_net( &(ptr[i]->address.net6),
					   &(ptr[i]->netmask.mask6),
					   &ipv6) )
		    continue;
#endif
	    }
	    if ( pos != NULL )
		*pos = i;
	    if ( iponly != NULL )
		free(iponly);
	    return ptr[i];
	}
    }

 ERR_END:
    if ( iponly != NULL )
	free(iponly);
    if ( pos != NULL )
	*pos = 0;
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
_exclude_find_free(int *nospace) {
    excluderecord_t **ptr;
    unsigned long i;

    assert ( nospace != NULL );

    ptr = exclude_vector;
    for (i = 0; i < er_vector_size; i++ ) {
	if ( ptr[i] == NULL ) {
	    *nospace = RETVAL_OK;
	    return i;
	}
    }
    *nospace = 1;
    return RETVAL_OK;
}

void
exclude_init() {
    int retval;

    if (initialized)
	return;

    exclude_vector = (excluderecord_t**)malloc( sizeof(excluderecord_t*) * er_vector_size);
    if ( exclude_vector == NULL ) {
	out_err("Unable to locate memory for the exclude record vector. Dying now");
	exit (3);
    }
    memset ( exclude_vector, 0, sizeof(excluderecord_t*) * er_vector_size);

    retval = pthread_mutex_init(&exclude_mutex, NULL);
    if ( retval != 0 ) {
	out_syserr(errno, "Error initializing exclude_mutex");
	exit (3);
    }

    initialized = 1;
}

void
exclude_destroy () {
    unsigned long i;
    excluderecord_t **ptr;
    int retval;

    if (!initialized)
	return;

    ptr = exclude_vector;
    for (i = 0; i < er_vector_size; i++) {
	if ( ptr[i] != NULL ) {
	    free(ptr[i]);
	}
    }
    free(ptr);

    retval = pthread_mutex_destroy ( &exclude_mutex );
    if ( retval != 0 )
	out_syserr(errno, "Error destroying exclude_mutex" );
}

int
exclude_clear() {
    int retval,i;
    excluderecord_t **ptr;

    if (!initialized)
	return RETVAL_ERR;

    retval = pthread_mutex_lock(&exclude_mutex);
    if ( retval != 0 ) {
	out_syserr(errno, "Unable to lock exclude_mutex");
	return RETVAL_ERR;
    }


    ptr = exclude_vector;
    for (i = 0; i < er_vector_size; i++) {
	if ( ptr[i] != NULL ) {
	    free(ptr[i]);
	}
    }
    free(ptr);

    er_vector_size = 100;
    er_vector_fill = 0;

    exclude_vector = (excluderecord_t**)malloc( sizeof(excluderecord_t*) * er_vector_size);
    if ( exclude_vector == NULL ) {
	out_err("Unable to locate memory for the exclude record vector. Dying now");
	exit (3);
    }
    memset ( exclude_vector, 0, sizeof(excluderecord_t*) * er_vector_size);

    retval = pthread_mutex_unlock(&exclude_mutex);
    if ( retval != 0 ) {
	out_syserr(errno, "Unable to unlock exclude_mutex");
	return RETVAL_ERR;
    }
    return RETVAL_OK;
}

/***
 * Adds an IP Address to the exclude list. It supports the following syntax:
 *
 * - <IPv4/IPv6>/<prefix>
 * - <IPv4/IPv6>
 *
 * If <prefix> is not given, it will assume 32 bit prefix for IPv4 or 128 bit
 * prefix for IPv6.
 *
 * @return
 * 0 on success, -1 on error.
 */
int
exclude_add(const char *ipaddr) {
    int retval;
    unsigned long pos_newexclude;
    excluderecord_t *ptr;
    int af;
    unsigned int prefix;
    char *iponly = NULL;
    in_addr_t ipv4, mask;
#ifdef HAVE_IN6_ADDR_T
    in6_addr_t ipv6;
#endif

    if (ipaddr == NULL) return RETVAL_ERR;

    /*
     * We need exclusive access to the vector, since we're going to modify it
     */
    retval = pthread_mutex_lock(&exclude_mutex);
    if ( retval != 0 ) {
	out_syserr(errno, "Unable to lock exclude_mutex");
	return RETVAL_ERR;
    }

    retval = _exclude_parse_ip(ipaddr, &iponly, &prefix, &af);
    if ( retval == RETVAL_ERR ) {
	out_err("Error parsing '%s' into components", ipaddr);
	goto END_ERR;
    }

    if ( af == AF_INET ) {
	retval = inet_pton(af, iponly, &ipv4);
	if ( retval != 1 ) {
	    out_syserr(errno, "Error translating '%s' to numeric address", iponly);
	    goto END_ERR;
	}
    } else {
#ifdef HAVE_IN6_ADDR_T
	retval = inet_pton(af, iponly, &ipv6);
	if ( retval != 1 ) {
	    out_syserr(errno, "Error translating '%s' to numeric address", iponly);
	    goto END_ERR;
	}
#endif
    }

    pos_newexclude = _exclude_find_free(&retval);
    if ( pos_newexclude == 0 && retval != RETVAL_OK ) {
	pos_newexclude = _exclude_vector_grow();
    }

    exclude_vector[pos_newexclude] =
	(excluderecord_t*) malloc ( sizeof(excluderecord_t) );
    if (exclude_vector[pos_newexclude] == NULL) {
	out_err("Unable to allocate space for exclude record");
	goto END_ERR;
    }

    exclude_vector[pos_newexclude]->af = af;
    if ( af == AF_INET ) {
	exclude_vector[pos_newexclude]->address.net = ipv4;
	mask = 0xffffffff << 32-prefix;
	exclude_vector[pos_newexclude]->netmask.mask = htonl(mask);
    } else {
#ifdef HAVE_IN6_ADDR_T
	exclude_vector[pos_newexclude]->address.net6 = ipv6;
	retval = _exclude_prefix_to_ipv6_mask
	    (
	     prefix,
	     &(exclude_vector[pos_newexclude]->netmask.mask6)
	     );
	if ( retval == RETVAL_ERR ) {
	    out_err("Unable to calculate netmask for prefix /%d", prefix);
	    goto END_ERR;
	}
#endif
    }

    er_vector_fill =  (pos_newexclude+1) > er_vector_fill ? (pos_newexclude+1) : er_vector_fill;

 END_OK:
    retval = pthread_mutex_unlock(&exclude_mutex);
    if ( retval != 0 ) {
	out_syserr(errno, "Unable to unlock exclude_mutex");
	return RETVAL_ERR;
    }
    if ( iponly != NULL )
	free(iponly);
    return RETVAL_OK;
 END_ERR:
    retval = pthread_mutex_unlock(&exclude_mutex);
    if ( retval != 0 ) {
	out_syserr(errno, "Unable to unlock exclude_mutex");
	return RETVAL_ERR;
    }
    if ( iponly != NULL )
	free(iponly);
    return RETVAL_ERR;
}

int
exclude_isexcluded(const char* ipaddr) {
    int retval, found;

    if (ipaddr == NULL ) return RETVAL_ERR;

    retval = pthread_mutex_lock(&exclude_mutex);
    if ( retval != 0 ) {
	out_syserr(errno, "Unable to lock exclude_mutex");
	return RETVAL_ERR;
    }

    found = _records_find(ipaddr, NULL) == NULL ? RETVAL_ERR : RETVAL_OK;

    retval = pthread_mutex_unlock(&exclude_mutex);
    if ( retval != 0 ) {
	out_syserr(errno, "Unable to unlock exclude_mutex");
	return RETVAL_ERR;
    }
    return found;
}

/**
 * Reads a file containing ip addresses/ranges to be excluded.
 *
 * It will create a thread for this task, since this function could be called
 * from a signal handler and end up in a dead lock if the main thread is using
 * the exclude vector at the same time.
 *
 * @param fpath the file to read
 *
 * @return 0 on success, else -1. errno will be set to the system error if any.
 */
int
exclude_readfile(const char* fpath) {
    int retval;
    pthread_attr_t tattr;


    retval = pthread_attr_init(&tattr);
    if (retval != 0) {
	out_syserr(errno, "Error initializing thread attributes for exclude file read thread");
	return RETVAL_ERR;
    }

    retval = pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
    if (retval != 0) {
	out_syserr(errno, "Error setting detach state for exclude file read thread");
	return RETVAL_ERR;
    }

    retval = pthread_create(NULL, &tattr, _exclude_readfile_thread, (void*)fpath);
    if (retval != 0) {
	out_syserr(errno, "Error lauching exclude file read thread");
	return RETVAL_ERR;
    }

    return RETVAL_OK;
}
