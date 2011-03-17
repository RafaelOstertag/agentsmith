
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

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifdef HAVE_SEMAPHORE_H
#include <semaphore.h>
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

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "globals.h"
#include "cfg.h"
#include "server.h"
#include "worker.h"
#include "output.h"
#include "netshared.h"

enum {
    /*
     * The time we wait for worker threads to end their job.
     * After that time has elapsed, the worker thread is spawned anyway. 
     */
    MAXWAITWORKER = 10
};

static int network_server_initialized = 0;

/**
 * This is used for select
 */
fd_set    fd_set_listen;

/**
 * The fd for each socket.
 */
int       listen_fds[FD_SETSIZE];

/**
 * The number of entries of fds.
 */
int       used_fds;

/**
 * Initialize all the sockets and data structures needed for providing the
 * server part of agentsmith.
 *
 * Do not use SO_SNDTIMEO and SO_RCVTIMEO on socket. It will crash SSL.
 */
static int
_network_start_listening() {
    char      host[NI_MAXHOST];
    char      serv[NI_MAXSERV];
    int       sockfd, retval, optval;
    struct addrinfo *ai;
    addrinfo_list_t *ptr;

    memset(listen_fds, 0, sizeof (int) * FD_SETSIZE);
    FD_ZERO(&fd_set_listen);

    used_fds = 0;
    ptr = CONFIG.listen;
    while (ptr != NULL && used_fds < FD_SETSIZE) {
	ai = ptr->addr;
	while (ai != NULL) {
	    /*
	     * Just in case we need it 
	     */
	    getnameinfo(ai->ai_addr, ai->ai_addrlen,
			host, NI_MAXHOST,
			serv, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);

	    out_dbg("Server: trying to open socket for %s port %s", host,
		    serv);

	    sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	    if (sockfd == -1) {
		out_syserr(errno, "Server: failed call to socket()");
		ai = ai->ai_next;
		continue;
	    }

	    optval = 1;
	    retval =
		setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval,
			   sizeof (int));
	    if (retval != 0) {
		out_syserr(errno,
			   "Server: failed to set SO_REUSEADDR on sock fd %i",
			   sockfd);
	    }

	    retval = bind(sockfd, ai->ai_addr, ai->ai_addrlen);
	    if (retval != 0) {
		out_syserr(errno, "Server: failed to bind to %s:%s", host,
			   serv);
		close(sockfd);
		ai = ai->ai_next;
		continue;
	    }

	    retval = listen(sockfd, CONFIG.server_backlog);
	    if (retval != 0) {
		out_syserr(errno, "Server: failed to listen on %s:%s", host,
			   serv);
		close(sockfd);
		ai = ai->ai_next;
		continue;
	    }

	    listen_fds[used_fds] = sockfd;
	    FD_SET(sockfd, &fd_set_listen);

	    out_dbg("Server: listening on sockfd %i", sockfd);

	    used_fds++;
	    ai = ai->ai_next;
	}
	ptr = ptr->next;
    }

    network_server_initialized = 1;
    return RETVAL_OK;
}

static void
_network_server_shutdown_and_cleanup(void *wdc) {
#ifdef HAVE_NANOSLEEP
    struct timespec time_wait, time_wait_remaining;
#endif
    int       i, retval, n;

    if (!network_server_initialized)
	return;

    out_dbg("Server: thread is shutting down");

    for (i = 0; i < used_fds; i++) {
	out_dbg("Server: shutting down fd %i", listen_fds[i]);
	close(listen_fds[i]);
    }

    n = 1;
    while (((retval =
	     sem_getvalue(&worker_semaphore, &i),
	     i)) < CONFIG.maxinconnections) {
	if (retval != 0) {
	    out_syserr(errno, "Server: error getting worker_semaphore value");
	    break;
	}

	out_dbg("Server: worker_semaphore: value %i, wait until value is %i",
		i, CONFIG.maxinconnections);
#ifdef HAVE_NANOSLEEP
	time_wait.tv_sec = 1;
	time_wait.tv_nsec = 0;
	nanosleep(&time_wait, &time_wait_remaining);
#else
#warning "Using sleep()"
	sleep(1);
#endif

	if (n >= MAXWAITWORKER) {
	    out_msg
		("Server: Continue shutdown of server, ignoring active worker threads");
	    break;
	}
	n++;
    }

    memset(listen_fds, 0, sizeof (listen_fds));
    FD_ZERO(&fd_set_listen);

    retval = sem_destroy(&worker_semaphore);
    if (retval != 0) {
	out_syserr(errno, "Server: error destroying worker_semaphore");
	return;
    }
    network_server_initialized = 0;
}

/**
 * Starts the agentsmith daemon. It expects to be launched from a thread.
 */
int
network_start_server() {
    char      host[NI_MAXHOST];
    char      serv[NI_MAXSERV];
    fd_set    rset;
    int       retval, sem_val, i, n, errnobak;
    worker_thread_args_t *thr_args;
    socklen_t addrlen;
    void     *sock_addr;
    pthread_attr_t tattr;
    pthread_t wdc;
#ifdef HAVE_NANOSLEEP
    struct timespec time_wait, time_wait_remaining;
#endif

    if (network_server_initialized)
	return RETVAL_ERR;

    retval = pthread_attr_init(&tattr);
    if (retval != 0) {
	out_syserr(retval,
		   "Server: error initializing thread attributes for exclude file read thread");
	return RETVAL_ERR;
    }

    retval = pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
    if (retval != 0) {
	out_syserr(retval,
		   "Server: error setting detach state for exclude file read thread");
	return RETVAL_ERR;
    }

    retval = sem_init(&worker_semaphore, 0, CONFIG.maxinconnections);
    if (retval != 0) {
	out_syserr(errno, "Server: error initializing worker_semaphore");
	return RETVAL_ERR;
    }

    retval = _network_start_listening();
    if (retval != 0)
	return retval;

    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    pthread_cleanup_push(_network_server_shutdown_and_cleanup, NULL);

    /*
     * The big loop accepting incoming connections. It will be left only by canceling the thread.
     */
    for (;;) {

	rset = fd_set_listen;

	/*
	 * We test here, in case select() gets interrupted by a signal. The
	 * code will test specifically for EINTR and continue the loop from top
	 * if select is interrupted. 
	 */
	pthread_testcancel();

	retval = select(FD_SETSIZE, &rset, NULL, NULL, NULL);
	errnobak = errno;
	if (retval == -1) {
	    switch (errnobak) {
	    case EINTR:
		continue;
	    default:
		out_syserr(errnobak, "Server: Error in multiplexing");
		pthread_exit(NULL);
	    }
	}

	out_dbg("Server: %i connections pending", retval);

	for (i = 0; i < used_fds; i++) {
	    if (FD_ISSET(listen_fds[i], &rset)) {
		/*
		 * The memory has to be free'd by the thread 
		 */
		sock_addr = malloc(MYSOCKADDRLEN);
		if (sock_addr == NULL) {
		    out_err("Server: memory exhausted. Dying now");
		    exit(1);
		}

		/*
		 * Make sure we don't spawn to many threads 
		 */
		sem_val = 0;
		n = 1;
		while (((retval =
			 sem_getvalue(&worker_semaphore, &sem_val)),
			sem_val) < 1) {
		    if (retval != 0) {
			out_syserr(errno,
				   "Server: error getting worker_semaphore value");
			break;
		    }
		    out_err
			("Server: Already %i thread(s) active. Waiting for exit of running threads",
			 CONFIG.maxinconnections);
#ifdef HAVE_NANOSLEEP
		    time_wait.tv_sec = 1;
		    time_wait.tv_nsec = 0;
		    nanosleep(&time_wait, &time_wait_remaining);
#else

#warning "Using sleep()"
		    sleep(1);
#endif

		    if (n >= MAXWAITWORKER) {
			/*
			 * Remember, the connection will be accepted, but the
			 * thread might be blocked while waiting for the
			 * semaphore once spawned (see worker thread
			 * implementation). This is the ideal point for a
			 * DOS. Create enough idle connections to the server,
			 * so agentsmith connections won't come thru.
			 */
			out_err
			    ("Server: error waiting for exit of running worker threads. Spawning new");
			break;
		    }
		    n++;
		}

		/*
		 * Accept the connection 
		 */
		addrlen = MYSOCKADDRLEN;
		retval = accept(listen_fds[i], sock_addr, &addrlen);
		if (retval == -1) {
		    out_syserr(errno, "Server: error accepting connection");
		    continue;
		}

		/*
		 * This is used for writing a log entry 
		 */
		getnameinfo(sock_addr, addrlen,
			    host, NI_MAXHOST,
			    serv, NI_MAXSERV,
			    NI_NUMERICHOST | NI_NUMERICSERV);
		out_msg("Server: accepting connection from %s port %s", host,
			serv);

		/*
		 * The memory has to be free'd by the thread 
		 */
		thr_args = (worker_thread_args_t *)
		    malloc(sizeof (worker_thread_args_t));
		if (thr_args == NULL) {
		    out_err("Server: memory exhausted. Dying now");
		    exit(1);
		}

		thr_args->connfd = retval;
		thr_args->addrlen = addrlen;
		thr_args->addr = sock_addr;

		retval =
		    pthread_create(&wdc, &tattr, network_server_worker,
				   (void *) thr_args);
		if (retval != 0) {
		    out_syserr(retval,
			       "Server: error lauching network server worker thread");
		}
	    }
	}			/* for (i=0; i < used_fds; i++) */

    }				/* for (;;) */

    pthread_cleanup_pop(1);

    /*
     * NOTREACHED 
     */
}
