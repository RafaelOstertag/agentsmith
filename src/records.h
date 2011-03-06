
/* Copyright (C) 2010, 2011 Rafael Ostertag
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

#include "globals.h"

/**
 * The modes available for enumerating records. Currently synchronous and
 * asynchronous mode is supported.
 */
enum enum_mode {
    SYNC,
    ASYNC
};
typedef enum enum_mode enum_mode_t;

/**
 * This is the host record holding information about ip addresses. It is also
 * shared between server and client.
 *
 * !!! Please note, that when changing the order of the members, change also
 * net_command_to_buff() and net_buff_to_command() in netshared.c
 */
struct _hostrecord {
    /*
     * The origin of this record. May be localhost or an IP address of the
     * agentsmith daemon sent this record 
     */
    char      origin[IPADDR_SIZE];
    char      ipaddr[IPADDR_SIZE];
    /*
     * When the ip address was first seen 
     */
    int64_t   firstseen;
    /*
     * When the ip address was last seen 
     */
    int64_t   lastseen;
    /*
     * How much time must elapse until the record is purged from memory 
     */
    int64_t   purge_after;
    /*
     * The subsequent occurrences must happen in this time interval 
     */
    int64_t   time_interval;
    /*
     * The number of occurrences of the ip address 
     */
    int32_t   occurrences;
    /*
     * This is the threshold of occurrences until the action is
     * performed 
     */
    int32_t   action_threshold;
    /*
     * If this is set != 0, the record will be removed when
     * records_maintenance()  
     */
    int32_t   remove;
    /*
     * Will be set when the record has been processed by the
     * action_thread() 
     */
    int32_t   processed;
};
typedef struct _hostrecord hostrecord_t;
#define size_hostrecord_t (4*8+4*4+2*46)

/**
 * The callback function which gets called when records are enumerated.
 *
 * If the callback function returns a value != 0, the enumeration stops.
 */
typedef int (*records_enum_callback) (hostrecord_t *);

/**
 * The callback function which gets called before record_maintenance() is
 * removing a record.
 */
typedef void (*records_remove_callback) (hostrecord_t *);

extern pthread_mutex_t vector_mutex;
extern void records_init();
extern void records_destroy(records_remove_callback cb);

/*
 * Performs maintenance on records, i.e. tries to free space and removes
 * records that are marked for deletion.
 */
extern int records_maintenance(records_remove_callback cb);

/* This is supposed to be called from the local agentsmith */
extern int records_add_ip(const char *ipaddr);

/* this is supposed to be called from a server worker thread */
extern int records_add_record(const hostrecord_t *hr);
extern int records_remove(const char *ipaddr);
extern int records_enumerate(records_enum_callback cb, enum_mode_t mode);
extern hostrecord_t *records_get(const char *ipaddr);

extern unsigned long records_dbg_get_vector_size();
extern unsigned long records_dbg_get_vector_chunksize();
extern unsigned long records_dbg_get_vector_fill();

#endif
