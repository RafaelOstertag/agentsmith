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

#ifndef RECORDS_H
#define RECORDS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

enum {
    /* Should be sufficient even for IPv6 */
    IPADDR_SIZE = 46
};

/**
 * The modes available for enumerating records. Currently synchronous and
 * asynchronous mode is supported.
 */
enum enum_mode {
    SYNC,
    ASYNC
} ;
typedef enum enum_mode enum_mode_t;

struct _hostrecord {
	char ipaddr[IPADDR_SIZE];
	time_t firstseen;
	time_t lastseen;
	int occurrences;
	/* If this is set != 0, the record will be removed when
	   records_maintenance()
	*/
	int remove;
	/* Will be set when the record has been processed by the action_thread() */
	int processed;
};
typedef struct _hostrecord hostrecord_t;

/*
 * The callback function which gets called when records are enumerated.
 *
 * If the callback function returns a value != 0, the enumeration stops.
 */
typedef int (*records_enum_callback)(hostrecord_t*);

/*
 * The callback function which gets called before record_maintenance() is
 * removing a recor.
 */
typedef void (*records_remove_callback)(hostrecord_t*);

extern pthread_mutex_t vector_mutex;
extern void records_init();
extern void records_destroy(records_remove_callback cb);
/* Performs maintenance on records, i.e. tries to free space and removes
 * records that are marked for deletion.
 */
extern int records_maintenance(records_remove_callback cb);
extern int records_add(const char *ipaddr);
extern int records_remove(const char *ipaddr);
extern int records_enumerate(records_enum_callback cb, enum_mode_t mode);
extern hostrecord_t *records_get(const char *ipaddr);

extern unsigned long records_dbg_get_vector_size();
extern unsigned long records_dbg_get_vector_chunksize();
extern unsigned long records_dbg_get_vector_fill();

#endif
