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
 */

/* $Id$ */

#ifndef EXCLUDE_H
#define EXCLUDE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#if defined(HAVE_STRUCT_IN6_ADDR) && !defined(HAVE_IN6_ADDR_T)
typedef struct in6_addr in6_addr_t;
#define HAVE_IN6_ADDR_T 1
#endif

struct _excluderecord {
    int       af;
    union {
#ifdef HAVE_IN_ADDR_T
	in_addr_t net;
#endif
#ifdef HAVE_IN6_ADDR_T
	in6_addr_t net6;
#endif
    } address;
    union {
#ifdef HAVE_IN_ADDR_T
	in_addr_t mask;
#endif
#ifdef HAVE_IN6_ADDR_T
	in6_addr_t mask6;
#endif
    } netmask;
};
typedef struct _excluderecord excluderecord_t;

extern void exclude_init();
extern void exclude_destroy();
extern int exclude_clear();
extern int exclude_add(const char *ipaddr);
extern int exclude_isexcluded(const char *ipaddr);
extern int exclude_readfile(const char *fpath);

extern unsigned long exclude_dbg_get_vector_size();
extern unsigned long exclude_dbg_get_vector_chunksize();
extern unsigned long exclude_dbg_get_vector_fill();

#endif
