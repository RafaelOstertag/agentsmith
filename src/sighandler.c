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

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include "output.h"
#include "follow.h"
#include "sighandler.h"
#include "records.h"
#include "cfg.h"
#include "exclude.h"

/* We keep the previous signal set, just in case */
static sigset_t old_sigset;

static int
_records_callback_output(hostrecord_t *ptr) {
    char *format = "IP Addr %s first seen %s, last seen %s, occurrences %i, to be removed %i, processed %i";
#ifdef HAVE_CTIME_R
#ifdef DEBUG
#warning "Using ctime_r()"
#endif
#define TIMEBUFSIZE 128
    char timebuff1[TIMEBUFSIZE], timebuff2[TIMEBUFSIZE];
#else
#ifdef DEBUG
#warning "Using ctime()"
#endif
#endif
    assert( ptr != NULL );

#ifdef HAVE_CTIME_R
    ctime_r(&(ptr->firstseen), timebuff1);
    ctime_r(&(ptr->lastseen), timebuff2);
    /* Get rid of the newlines */
    timebuff1[strlen(timebuff1)-1]='\0';
    timebuff2[strlen(timebuff2)-1]='\0';
    out_msg(format,
	   ptr->ipaddr,
	   timebuff1,
	   timebuff2,
	   ptr->occurrences,
	   ptr->remove,
	   ptr->processed);
#else
    out_msg(format,
	   ptr->ipaddr,
	   ctime(ptr->firstseen),
	   ctime(ptr->lastseen),
	   ptr->occurrences,
	   ptr->remove,
	   ptr->processed);
#endif
    return 0;
}

static int
_records_callback_remove_all(hostrecord_t *ptr) {
    assert (ptr!=NULL);
    ptr->remove = 1;
    return 0;
}

static void
sighandler_unhandled(int no) {
    int sav_errno;
    
    sav_errno = errno;
    out_msg("Received unhandled signal %i. Ignoring", no);

    errno = sav_errno;
}

static void
signalhandler_fatal(int no) {
    out_err("Ouch, received fatal signal %i. Dying now", no);
    exit(2);
}

static void
signalhandler_termination(int no) {
    out_err("Received termination signal %i. Going down now", no);
    follow_stop();
}

static void
signalhandler_usr1(int no) {
    int retval, sav_errno;

    assert(no == SIGUSR1);

    sav_errno = errno;
    out_msg("Received signal USR1. Output of host records follows...");
    retval = records_enumerate(_records_callback_output, ASYNC);
    if (retval != 0)
	out_err("Error enumerating records for output");

    errno=sav_errno;
}

static void
signalhandler_usr2(int no) {
    int retval, sav_errno;

    assert(no == SIGUSR2);

    sav_errno = errno;
    out_msg("Received signal USR2. Host records will be purged upon the next run of the maintenance thread");
    retval = records_enumerate(_records_callback_remove_all, ASYNC);
    if (retval != 0)
	out_err("Error enumerating records for purging");

    errno = sav_errno;
}

static void
signalhandler_hup(int no) {
    int retval, sav_errno;
    config* cfg;

    assert(no == SIGHUP);

    sav_errno = errno;
    out_msg("Received signal HUP. Re-read exclude file");

    cfg = config_get();
    if (cfg == NULL) {
	out_err("Unable to get configuration for re-reading exclude file");
	return;
    }

    retval = exclude_readfile(cfg->exclude);
    if (retval != 0)
	out_syserr(errno, "Error re-reading exclude file %s", cfg->exclude);

    errno = sav_errno;
}

void
signalhandler_setup() {
    sigset_t our_set;
    struct sigaction sa;
    int retval;

    sigemptyset(&our_set);
    retval = sigprocmask(SIG_SETMASK, &our_set, &old_sigset);
    if ( retval == -1 ){
	out_syserr(errno, "Error settting sigprocmask()");
	abort();
    }

    /* The ignored signals */
    sigfillset(&(sa.sa_mask));
#ifdef SA_RESTART
    sa.sa_flags = SA_RESTART;
#else
    sa.sa_flags = 0;
#endif
    sa.sa_handler = sighandler_unhandled;
    retval = sigaction ( SIGALRM, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }
    retval = sigaction ( SIGCONT, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }
    retval = sigaction ( SIGTSTP, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }
#ifdef SIGINFO
    retval = sigaction ( SIGINFO, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }
#endif
    retval = sigaction ( SIGIO, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }
    retval = sigaction ( SIGPIPE, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }
#ifdef SIGPOLL
    retval = sigaction ( SIGPOLL, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }
#endif
    retval = sigaction ( SIGPROF, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }
    retval = sigaction ( SIGTTIN, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }
    retval = sigaction ( SIGTTOU, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }
    retval = sigaction ( SIGURG, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }
    retval = sigaction ( SIGVTALRM, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }
    retval = sigaction ( SIGWINCH, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }

    /* The fatal signals */
    sigfillset(&(sa.sa_mask));
    sa.sa_flags = 0;
    sa.sa_handler = signalhandler_fatal;
    retval = sigaction ( SIGABRT, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }
    retval = sigaction ( SIGBUS, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }
#ifdef SIGEMT
    retval = sigaction ( SIGEMT, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }
#endif
    retval = sigaction ( SIGFPE, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }
    retval = sigaction ( SIGILL, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }
    retval = sigaction ( SIGIOT, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }
    retval = sigaction ( SIGSEGV, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }
    retval = sigaction ( SIGSYS, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }
    retval = sigaction ( SIGTRAP, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }
    retval = sigaction ( SIGXCPU, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }
    retval = sigaction ( SIGXFSZ, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }

    /* Termination signals  */
    sigfillset(&(sa.sa_mask));
    sa.sa_flags = 0;
    sa.sa_handler = signalhandler_termination;
    retval = sigaction ( SIGQUIT, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }
    retval = sigaction ( SIGTERM, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }
    retval = sigaction ( SIGINT, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }

    /* Dump records */
    sigfillset(&(sa.sa_mask));
    sa.sa_flags = 0;
    sa.sa_handler = signalhandler_usr1;
    retval = sigaction ( SIGUSR1, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }
    /* Remove all records */
    sigfillset(&(sa.sa_mask));
    sa.sa_flags = 0;
    sa.sa_handler = signalhandler_usr2;
    retval = sigaction ( SIGUSR2, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }
    /* Reread exclude list */
    sigfillset(&(sa.sa_mask));
    sa.sa_flags = 0;
    sa.sa_handler = signalhandler_hup;
    retval = sigaction ( SIGHUP, &sa, NULL);
    if ( retval == -1 ) {
	out_syserr(errno, "Error setting up signal handler");
	abort();
    }
}
