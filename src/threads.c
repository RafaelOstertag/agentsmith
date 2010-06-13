/* Copyright (C) 2010 Rafael Ostertag
 *
 * This file is part of sshsentinel.
 *
 * sshsentinel is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Foobar is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Foobar.  If not, see <http://www.gnu.org/licenses/>.
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

#include "globals.h"
#include "threads.h"
#include "records.h"
#include "cfg.h"
#include "output.h"

static const int maintenance_sleep_time = 60000000;
static const int action_sleep_time = 500000;
static pthread_t action_thread_id;
static pthread_t maintenance_thread_id;

static void
_do_action(const hostrecord_t *ptr, const config *cfg) {
    char action[BUFFSIZE];
    int retval;

    retval = access (cfg->action, R_OK | X_OK | F_OK);
    if ( retval == -1 ) {
	out_syserr(errno, "Unable to launch '%s'.", cfg->action);
	return;
    }

    snprintf(action, BUFFSIZE, "%s %s %i",
	     cfg->action,
	     ptr->ipaddr,
	     ptr->occurrences);
    out_dbg("Executing: %s", action);
    retval = system(action);
    out_dbg("'%s' returned %i", action, retval);
    if ( retval == -1 ) {
	out_err("'%s' failed with exit code %i", action, errno);
	return;
    }
    if ( retval != 0 ) {
	out_err("'%s' returned %i", action, retval);
    }
}

static int
_records_callback(hostrecord_t *ptr) {
    config *cfg;

    assert( ptr != NULL);
    cfg = config_get();
    if ( cfg == NULL ) {
	out_err("<NULL> configuration received while enumerating records!.");
	return 0;
    }

    if ( ptr->occurrences >= cfg->action_threshold &&
	 ptr->processed == 0) {
	_do_action(ptr, cfg);
	ptr->processed = 1;
	return 0;
    }

    /* Is this record a one time record? */
    if ( ptr->lastseen == 0 ) {
	if ( (time(NULL) - ptr->firstseen) > cfg->purge_after ) {
	    ptr->remove = 1;
	    return 0;
	}
    } else {
	/* Remove stale records */
	if ( time(NULL) - ptr->lastseen  > cfg->purge_after ) {
	    ptr->remove = 1;
	    return 0;
	}
    }
    return 0;
}

static void*
action_thread(void *wdc) {
    int retval;
    for (;;) {
	retval = records_enumerate(_records_callback);
	if (retval != 0)
	    out_err("records_enumerate() gave an error. Continuing...");

	usleep(action_sleep_time);
	pthread_testcancel();
    }
    return NULL;
}

static void*
maintenance_thread(void *wdc) {
    int retval;

    for (;;) {

	out_dbg("Starting records maintenance");
	retval = records_maintenance();
	out_dbg("Ending records maintenance");

	usleep(maintenance_sleep_time);
	pthread_testcancel();
    }
    return NULL;
}

void
threads_start() {
    int retval;

    retval = pthread_create(&action_thread_id, NULL, action_thread, NULL);
    if (retval != 0) {
	out_syserr(errno, "Error creating action thread.");
	exit (1);
    }

    retval = pthread_create(&maintenance_thread_id,
			    NULL,
			    maintenance_thread,
			    NULL);
    if (retval != 0) {
	out_syserr(errno, "Error creating maintenance thread.");
	exit (1);
    }
}

void
threads_stop() {
    int retval;

    out_msg("Cancelling maintenance thread");
    retval = pthread_cancel(maintenance_thread_id);
    if (retval != 0) {
	out_syserr(errno, "Error cancelling maintenance thread");
    }
    retval = pthread_join(maintenance_thread_id, NULL);
    if (retval != 0) {
	out_syserr(errno, "Error joining maintenance thread");
    }

    out_msg("Cancelling action thread");
    retval = pthread_cancel(action_thread_id);
    if (retval != 0) {
	out_syserr(errno, "Error cancelling action thread");
    }
    retval = pthread_join(action_thread_id, NULL);
    if (retval != 0) {
	out_syserr(errno, "Error joining action thread");
    }

}
