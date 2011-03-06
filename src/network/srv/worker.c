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

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_SEMAPHORE_H
#include <semaphore.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "globals.h"
#include "cfg.h"
#include "server.h"
#include "output.h"
#include "worker.h"
#include "exclude.h"
#include "netshared.h"

enum {
    MAX_LOCK_TRIES=5
};

void*
network_server_worker(void *args) {
    char host[NI_MAXHOST];
    char serv[NI_MAXSERV];
    char buff[REMOTE_COMMAND_SIZE];
    uint32_t rcommand;
    int retval, lock_tries;
    ssize_t nread;
    worker_thread_args_t *wrk_args;
    hostrecord_t hostrecord;
#ifdef KERNEL_SUNOS
    fd_set readready;
    int saverrno;
    struct timeval timeout;
#endif

    assert ( args != NULL );

    wrk_args = (worker_thread_args_t*)args;

    /* How many times we wait for the semaphore */
    lock_tries = 0;

    getnameinfo(wrk_args->addr, wrk_args->addrlen,
		host, NI_MAXHOST,
		serv, NI_MAXSERV,
		NI_NUMERICHOST | NI_NUMERICSERV);


    /*
     * Try to lock the semaphore. If the semaphore cannot be locked, sleep one
     * second. Try it MAX_LOCK_TRIES at max. If no lock can be aquired, end the
     * thread.
     */
 REDO:
    lock_tries++;
    if (lock_tries >= MAX_LOCK_TRIES) {
	out_err("Server Worker [%li]: unable to lock worker_semaphore. Aborting after %i attempts",
		pthread_self(), MAX_LOCK_TRIES);
	goto ENDNOPOST;
    }

    retval = sem_trywait(&worker_semaphore);
    switch (errno) {
	case EAGAIN:
	    out_dbg("Server Worker [%li]: cannot lock worker_semaphore. Retrying...",
		    pthread_self());
	    sleep(1);
	    goto REDO;
	case 0:
	    break;
	default:
	    out_syserr(errno, "Server Worker [%li]: error locking worker_semaphore",
		       pthread_self());
	    goto ENDNOPOST;
    }

#ifdef KERNEL_SUNOS /* SunOS does not support the SO_RCVTIMEO socket option, so
		       we have to use select */
#ifdef DEBUG
#warning "++++ Compiling in code for SUNOS ++++"
#endif
    timeout.tv_sec = CONFIG.server_timeout;
    timeout.tv_usec = 0;

    FD_ZERO(&readready);
    FD_SET(wrk_args->connfd,&readready);

    while ( (retval = select(wrk_args->connfd+1, &readready, NULL, NULL, &timeout)) < 0 ) {
	saverrno = errno;
	if ( saverrno == EINTR ) {
	    out_dbg("Server Worker [%li]: select on read was interrupted. Retrying...",
		    pthread_self());
	    continue;
	}
	
	out_syserr(saverrno, "Server Worker [%li]: error in select",
		   pthread_self());
	goto END;
    }

    if ( retval == 0 ) {
	out_err("Server Worker [%li]: time out waiting for data from %s port %s",
		pthread_self(), host, serv);
	goto END;
    }

    if (FD_ISSET(wrk_args->connfd, &readready) == 0) {
	out_err("Server Worker [%li]: data is ready for reading, but not on connfd %i",
		pthread_self(), wrk_args->connfd);
	goto END;
    }
#endif
	
    while ( (nread = net_read(wrk_args->connfd, buff, REMOTE_COMMAND_SIZE, &retval)) !=0 ) {
	if ( nread == RETVAL_ERR ) {
	    out_syserr(retval, "Server Worker [%li]: error reading from client %s port %s",
		       pthread_self(), host, serv);
	    goto END;
	}

	if ( nread != REMOTE_COMMAND_SIZE ) {
	    out_err("Server Worker [%li]: read error: expected %i bytes, read %i bytes",
		    pthread_self(), REMOTE_COMMAND_SIZE, nread);
	    goto END;
	}

	/* Translate the buffer to the command and host
	   record. Little/Bigendian transformation is taken care of by this
	   function */
	net_buff_to_command(buff, &rcommand, &hostrecord);

	switch (rcommand) {
	    case ADD:
		out_dbg("Server Worker [%li]: remote command: ADD", pthread_self());

		if ( CONFIG.remote_authoritative == 0 &&
		     exclude_isexcluded(hostrecord.ipaddr) == RETVAL_OK) {
		    out_msg("Server Worker [%li]: Ignoring %s received from %s port %s due to exclusion",
			    pthread_self(), hostrecord.ipaddr,host,serv);
		    break;
		}

		hostrecord.remove = 0; /* We want to have this value */
		hostrecord.processed = 0; /* We want to have this value */

		/* Copy the clients ip address to the record's origin field */
		strncpy(hostrecord.origin, host, IPADDR_SIZE);
		hostrecord.origin[IPADDR_SIZE - 1] = '\0';

#ifdef DEBUG
		out_dbg("Server Worker [%li]: Received remote command...", 
			pthread_self());
		__dbg_dump_host_record(&hostrecord);
#endif

		assert(hostrecord.occurrences >= hostrecord.action_threshold);

		/*
		 * Insert the received record in our local list
		 */
		retval = records_add_record(&(hostrecord));
		if ( retval == RETVAL_ERR ) {
		    out_err("Server Worker [%li]: Unable to add record for '%s' received from %s port %s",
			    pthread_self(), hostrecord.ipaddr, host, serv);
		}

		/*
		 * Log the addition of the host record
		 */
		out_msg("Server Worker [%li]: Received record for IP Address %s from %s port %s",
			pthread_self(),
			hostrecord.ipaddr,
			host, serv);
		break;
	    case REMOVE:
#ifdef DEBUG
		out_dbg("Server Worker [%li]: remote command: REMOVE",
			pthread_self());
#endif
		out_msg("Server Worker [%li]: REMOVE command not implemented",
			pthread_self());
		break;
	    case INFORMATION:
#ifdef DEBUG
		out_dbg("Server Worker [%li]: remote command: INFORMATION",
			pthread_self());
#endif
		out_msg("Server Worker [%li]: INFORMATION command not implemented",
			pthread_self());
		break;
	    default:
		out_err("Server Worker [%li]: remote command not understood",
			pthread_self());
		goto END;
	}
    }

 END:
    retval = sem_post(&worker_semaphore);
    if ( retval != 0 )
	out_err("Server Worker [%li]: Unable to post worker_semaphore",
		pthread_self());
 ENDNOPOST:
        out_dbg("Server Worker [%li]: closing connection to %s port %s",
	    pthread_self(), host, serv);
    close(wrk_args->connfd);
    free(wrk_args->addr);
    free(wrk_args);

    pthread_exit(NULL);
}
