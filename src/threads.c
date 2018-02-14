
/* Copyright (C) 2010, 2011 by Rafael Ostertag
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
 *
 * In addition, as a special exception, the copyright holder give permission
 * to link the code of portions of this program with the OpenSSL library under
 * certain conditions as described in each individual source file, and
 * distribute linked combinations including the two.
 *
 * You must obey the GNU General Public License in all respects for all of the
 * code used other than OpenSSL.  If you modify file(s) with this exception,
 * you may extend this exception to your version of the file(s), but you are
 * not obligated to do so.  If you do not wish to do so, delete this exception
 * statement from your version.  If you delete this exception statement from
 * all source files in the program, then also delete it here.
 */

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif

#ifdef STDC_HEADERS
#include <stdlib.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef STDC_HEADERS
#include <string.h>
#endif

#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

#include "globals.h"
#include "threads.h"
#include "records.h"
#include "output.h"
#include "server.h"
#include "client.h"

/* In seconds */
static const long maintenance_sleep_time = 60;

/* In nano seconds */
static const long action_sleep_time = 500000000;
static pthread_t action_thread_id;
static pthread_t maintenance_thread_id;
static pthread_t server_thread_id;

enum {
    ACTION_BUFFER_SIZE = 2048
};

enum ac_type {
    NEW = 0,
    REMOVE
};
typedef enum ac_type ac_type_t;

#define AC_TYPE_NEW_STR "new"
#define AC_TYPE_REMOVE_STR "remove"
#define AC_TYPE_UNKNOWN_STR "unknown"

static void
_do_action(const hostrecord_t *ptr, ac_type_t t) {
    char action[ACTION_BUFFER_SIZE];
    char *type_str;
    int retval;

    retval = access(CONFIG.action, R_OK | X_OK | F_OK);
    if (retval == -1) {
        out_syserr(errno, "Unable to launch '%s'.", CONFIG.action);
        return;
    }

    /*
     * Get the action type string for passing to the script.
     */
    switch (t) {
        case NEW:
            type_str = strdup(AC_TYPE_NEW_STR);
            if (CONFIG.inform == 1 && CONFIG.inform_agents != 0) {
                if (strcmp(ptr->origin, LOCALHOST) != 0) {
                    /*
                     * Record from remote host, so we don't want to relay it
                     * further 
                     */
                    break;
                }

                /*
                 * Add it the client queue, for distribution to other
                 * agentsmiths 
                 */
                assert(strlen(ptr->ipaddr) > 2);
                retval = client_queue_record(ptr);
                if (retval != RETVAL_OK)
                    out_err("Error queueing client host record to be sent.");
            }
            break;
        case REMOVE:
            type_str = strdup(AC_TYPE_REMOVE_STR);
            break;
        default:
            type_str = strdup(AC_TYPE_UNKNOWN_STR);
            out_err("Action type %i is unknown.", t);
            assert(0); /* In debug mode we want to abort */
            break;
    }

    snprintf(action, ACTION_BUFFER_SIZE, "%s %s %i %s %s",
            CONFIG.action,
            ptr->ipaddr, ptr->occurrences, type_str, ptr->origin);
    free(type_str);

    out_dbg("Executing: %s", action);
    retval = system(action);
    out_dbg("'%s' returned %i", action, retval);
    if (retval == -1) {
        out_err("'%s' failed with exit code %i", action, errno);
        return;
    }
    if (retval != 0) {
        out_err("'%s' returned %i", action, retval);
    }
}

/**
 * This function does two things:
 *
 * 1) if the action_threshold set in the configuration is reached, it calls the
 *    action.
 *
 * 2) It sets the record remove flag if the purge_after time is reached. It
 *    does not, however, purge the records itself, but lets the maintenance
 *    thread do the job.
 */
static int
_records_callback_action_new(hostrecord_t *ptr) {

    assert(ptr != NULL);

    if (ptr->occurrences >= ptr->action_threshold && ptr->processed == 0) {
        _do_action(ptr, NEW);
        ptr->processed = 1;
        return 0;
    }

    /*
     * Is this record a one time record? 
     */
    if (ptr->lastseen == 0) {
        if ((time(NULL) - ptr->firstseen) > ptr->purge_after) {
            ptr->remove = 1;
            return 0;
        }
    } else {
        /*
         * Remove stale records 
         */
        if (time(NULL) - ptr->lastseen > ptr->purge_after) {
            ptr->remove = 1;
            return 0;
        }
    }
    return 0;
}

static void *
action_thread(void *wdc) {
    int retval;
#ifdef HAVE_NANOSLEEP
    struct timespec time_wait, time_wait_remaining;
#endif

    for (;;) {
        retval = records_enumerate(_records_callback_action_new, SYNC);
        if (retval != 0)
            out_err("records_enumerate() gave an error. Continuing...");

        /*
         * Flush the client host records, to make sure there is no unsent host record left 
         */
        if (CONFIG.inform == 1 && CONFIG.inform_agents != 0) {
            retval = client_queue_flush();
            if (retval != RETVAL_OK)
                out_err("Error flushing client host record queue.");
        }
#ifdef HAVE_NANOSLEEP
        time_wait.tv_sec = 0;
        time_wait.tv_nsec = action_sleep_time;
        nanosleep(&time_wait, &time_wait_remaining);
#else

#warning "Using usleep()"
        usleep(action_sleep_time / 1000);
#endif

        pthread_testcancel();
    }
    return NULL;
}

static void *
maintenance_thread(void *wdc) {
#ifdef HAVE_NANOSLEEP
    struct timespec time_wait, time_wait_remaining;
#endif

    for (;;) {
        out_dbg("Starting records maintenance");
        records_maintenance(threads_records_callback_action_removal);
        out_dbg("Ending records maintenance");

#ifdef HAVE_NANOSLEEP
        time_wait.tv_sec = maintenance_sleep_time;
        time_wait.tv_nsec = 0;
        nanosleep(&time_wait, &time_wait_remaining);
#else

#warning "Using usleep()"
        usleep(maintenance_sleep_time * 1000000);
#endif
        pthread_testcancel();
    }
    return NULL;
}

static void *
server_thread(void *wdc) {
    int retval;

    retval = network_start_server();
    if (retval == RETVAL_ERR)
        out_err("Unable to launch server thread");

    pthread_exit(NULL);
}

void
threads_start() {
    int retval;

    retval = pthread_create(&action_thread_id, NULL, action_thread, NULL);
    if (retval != 0) {
        out_syserr(retval, "Error creating action thread.");
        exit(1);
    }

    retval = pthread_create(&maintenance_thread_id,
            NULL, maintenance_thread, NULL);
    if (retval != 0) {
        out_syserr(retval, "Error creating maintenance thread.");
        exit(1);
    }

    if (CONFIG.server != 0) {
        retval = pthread_create(&server_thread_id, NULL, server_thread, NULL);
        if (retval != 0) {
            out_syserr(retval, "Error creating server thread.");
            exit(1);
        }
    }
}

void
threads_stop() {
    int retval;

    if (CONFIG.server != 0) {
        out_msg("Cancelling server thread");
        retval = pthread_cancel(server_thread_id);
        if (retval != 0) {
            out_syserr(retval, "Error cancelling server thread");
        }
        retval = pthread_join(server_thread_id, NULL);
        if (retval != 0) {
            out_syserr(retval, "Error joining action thread");
        }
    }

    out_msg("Cancelling maintenance thread");
    retval = pthread_cancel(maintenance_thread_id);
    if (retval != 0) {
        out_syserr(retval, "Error cancelling maintenance thread");
    }
    retval = pthread_join(maintenance_thread_id, NULL);
    if (retval != 0) {
        out_syserr(retval, "Error joining maintenance thread");
    }

    out_msg("Cancelling action thread");
    retval = pthread_cancel(action_thread_id);
    if (retval != 0) {
        out_syserr(retval, "Error cancelling action thread");
    }
    retval = pthread_join(action_thread_id, NULL);
    if (retval != 0) {
        out_syserr(retval, "Error joining action thread");
    }
}

/*
 * This function is exported because it is also called from main.c upon exit.
 */
void
threads_records_callback_action_removal(hostrecord_t *ptr) {

    assert(ptr != 0);

    /*
     * Only call the remove action if the record was processed 
     */
    if (ptr->processed != 0)
        _do_action(ptr, REMOVE);
}
