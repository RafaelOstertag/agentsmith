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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
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

#include "clientqueue.h"
#include "cfg.h"
#include "output.h"
#include "globals.h"
#include "netshared.h"

#define SERVERIPUNK "Server IP Unknown"
#define SERVERSERVUNK "Port unknown"
#define HOSTIPUNK "localhost"

static void
_client_queue_worker_cleanup(void* args) {
    queue_entry_t *ptr, *nextptr;

    out_dbg("Client Worker [%i]: in _client_queue_cleanup",
	       pthread_self());
    if ( args == NULL ) return;

    ptr = (queue_entry_t*) args;

    while ( ptr != NULL ) {
	nextptr = ptr->next;
	free(ptr->record);
	free(ptr);
	ptr = nextptr;
    }
    out_dbg("Client Worker [%i]: leaving _client_queue_cleanup",
	       pthread_self());
}

static void *
_client_queue_worker(void *arg) {
    char serverhost[NI_MAXHOST];
    char serverserv[NI_MAXSERV];
    char buff[REMOTE_COMMAND_SIZE];
    int retval, sockfd, retries=0, connect_success = 0;
    ssize_t writeres;
    uint32_t command;
    client_queue_t *queue;
    queue_entry_t *ptr, *nextptr;

    ptr = NULL;

    assert(arg != NULL);

    queue = (client_queue_t*) arg;

    /* Get server ip/service here, since it won't change during runtime of
       thread */
    retval = getnameinfo(queue->sockaddr, queue->socklen,
			 serverhost, NI_MAXHOST, 
			 serverserv, NI_MAXSERV,
			 NI_NUMERICHOST | NI_NUMERICSERV);
    if ( retval != 0 ) {
	out_err("Client Worker [%i]: Unable to get server's IP address (error: %s)",
		pthread_self(),
		gai_strerror(retval));
	strncpy(serverhost, SERVERIPUNK, NI_MAXHOST);
	serverhost[strlen(SERVERIPUNK)] = '\0';
	strncpy(serverserv, SERVERSERVUNK, NI_MAXSERV);
	serverserv[strlen(SERVERSERVUNK)] = '\0';
    }
    

    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    pthread_cleanup_push(_client_queue_worker_cleanup, ptr);

    /*
     * We stay in this loop until shutdown from the main thread
     */
    for (;;) {
	retval = pthread_mutex_lock(&(queue->mutex));
	if ( retval != 0 ) {
	    out_syserr(retval, "Client Worker [%i]: Unable to lock queue mutex",
		       pthread_self());
	    pthread_exit(NULL);
	}

	/* Wait for the condition, that the queue is no longer empty */
	while ( queue->head == NULL ) {
	    retval = pthread_cond_wait(&(queue->cond), &(queue->mutex) );
	    if ( retval != 0 ) {
		out_syserr(retval,
			   "Client Worker [%i]: Unable to wait for queue condition", pthread_self());
		pthread_exit(NULL);
	    }
	}


	ptr = queue->head; 	/* Take the entire queue */
	queue->head = NULL;     /* Mark the queue empty */

	retval = pthread_mutex_unlock(&(queue->mutex));
	if ( retval != 0 ) {
	    out_syserr(retval, "Client Worker [%i]: Unable to lock queue mutex",
		       pthread_self());
	    pthread_exit(NULL);
	}

	/* Try to connect to the server */
	connect_success = retries = 0;
	while (retries < CONFIG.inform_retry) {
	    retries++;

	    /* Connect to the server */
	    sockfd = socket(queue->family, SOCK_STREAM, 0);
	    if ( sockfd == -1 ) {
		out_syserr(errno, "Client Worker [%i]: Unable to open socket",
			   pthread_self());
		sleep(CONFIG.inform_retry_wait);
		continue;
	    }

	    retval = connect(sockfd, queue->sockaddr, queue->socklen);
	    if ( retval != 0 ) {
		out_syserr(errno,
			   "Client Worker [%i]: Unable to connect to %s port %s",
			   pthread_self(), serverhost, serverserv);
		sleep(CONFIG.inform_retry_wait);
		continue;
	    }
	    connect_success = 1;
	    break;
	}

	if (connect_success != 1) {
	    out_err("Client Worker [%i]: Unable to connecto to %s port %s. Tried %i times",
		       pthread_self(), serverhost, serverserv, retries);
	    _client_queue_worker_cleanup(ptr);	    
	    close(sockfd);
	    continue;
	}

	/* Process the queue */
	while ( ptr != NULL ) {
	    nextptr = ptr->next;
	    out_dbg("Client Worker [%i]: Processing queue entry",
		    pthread_self());

#ifdef DEBUG
	    out_dbg("Client Worker [%i]: Send remote command to %s port %s", 
		    pthread_self(), serverhost, serverserv);
	    __dbg_dump_host_record(ptr->record);
#endif

	    net_command_to_buff(ADD, ptr->record, buff, REMOTE_COMMAND_SIZE);

	    retval = 0;
	    writeres = net_write(sockfd, buff, REMOTE_COMMAND_SIZE, &retval);
	    if (writeres == RETVAL_ERR) 
		out_syserr(retval, "Client Worker [%i]: Error writing to %s port %s", pthread_self(), serverhost, serverserv);
	    /* In case of an error we just continue, since we have to free the
	       memory anyway */

	    free(ptr->record);
	    free(ptr);

	    ptr=nextptr;
	    pthread_testcancel();
	}

	close(sockfd);
    }

    pthread_cleanup_pop(1);

}

client_queue_t *
client_queue_init(int af, const struct sockaddr* s, socklen_t sl) {
    client_queue_t *newq;
    int retval;

    assert(s != NULL);
    assert(sl > 0);
    assert( af == AF_INET || af == AF_INET6 );

    newq = (client_queue_t*) malloc(sizeof(client_queue_t));
    if ( newq == NULL ) {
	out_err("Out of memory in client_queue_init(). Dying now");
	exit(1);
    }
    
    newq->family = af;
    newq->socklen = sl;

    newq->sockaddr = (struct sockaddr*) malloc(sl);
    if ( newq->sockaddr == NULL ) {
	out_err("Out of memory in client_queue_init(). Dying now");
	exit(1);
    }
    memcpy(newq->sockaddr, s, sl);

    newq->head = NULL;

    retval = pthread_mutex_init(&(newq->mutex), NULL);
    if ( retval != 0 ) {
	out_syserr(retval, "Error initializing mutex for client queue. Dying now");
	exit(1);
    }

    retval = pthread_cond_init(&(newq->cond), NULL);
    if ( retval != 0 ) {
	out_syserr(retval, "Error intializing conditional for client queue. Dying now");
	exit(1);
    }

    retval = pthread_create(&(newq->tid), NULL, _client_queue_worker, newq);
    if ( retval != 0 ) {
	out_syserr(retval, "Error creating queue worker thread");
	exit(1);
    }
    out_dbg("Created queue worker thread %i", newq->tid);

    return newq;
}

int
client_queue_destroy(client_queue_t *queue) {
    int retval;
    queue_entry_t *ptr, *next_ptr;

    assert(queue != NULL);

    out_dbg("Cancelling queue worker thread %i", queue->tid);
    retval = pthread_cancel(queue->tid);
    if (retval != 0)
	out_syserr(retval, "Error canceling queue worker thread %i", queue->tid);
    else {
	out_dbg("Joining queue worker thread %i", queue->tid);
	retval = pthread_join(queue->tid, NULL);
	if ( retval != 0 )
	    out_syserr(retval, "Error joining queue worker thread %i", queue->tid);
    }

    retval = pthread_cond_destroy(&(queue->cond));
    if ( retval != 0 )
	out_syserr(retval, "Error destroying client queue conditional");

    retval = pthread_mutex_destroy(&(queue->mutex));
    if ( retval != 0 )
	out_syserr(retval, "Error destroying client queue mutex");

    ptr = queue->head;
    while ( ptr != NULL ) {
	next_ptr = ptr->next;
	free(ptr->record);
	free(ptr);
	ptr = next_ptr;
    }

    free(queue->sockaddr);
    free(queue);

    return RETVAL_OK;
}

/**
 * Adds an array of \c len items to the queue
 *
 * @param queue pointer to the queue
 *
 * @param len the length of the array
 *
 * @param record pointer to the array of host records
 *
 * @return
 * \c RETVAL_OK on success, else \c RETVAL_ERR
 */
int
client_queue_append(client_queue_t *queue, const hostrecord_t **record, int len) {
    int retval, i;
    queue_entry_t *ptr;

    assert(queue != NULL);
    assert(record != NULL);
    assert(len > 0);

    retval = pthread_mutex_lock(&(queue->mutex));
    if ( retval != 0 ) {
	out_syserr(retval, "Client Worker [%i]: error locking queue mutex in client_queue_pop", pthread_self());
	return RETVAL_ERR;
    }

    ptr = queue->head;
    if ( ptr == NULL ) { /* The queue is empty */
	queue->head = (queue_entry_t*) malloc(sizeof(queue_entry_t));
	if ( queue->head == NULL ) {
	    out_err("Insufficient memory. Dying now");
	    exit(1);
	}

	queue->head->record = (hostrecord_t*) malloc(sizeof(hostrecord_t));
	if ( queue->head->record == NULL ) {
	    out_err("Insufficient memory. Dying now");
	    exit(1);
	}

	queue->head->next = NULL;
	memcpy(queue->head->record, record[0], sizeof(hostrecord_t));

	ptr = queue->head;

	for (i=1; i<len; i++) {
	    ptr->next = (queue_entry_t*) malloc(sizeof(queue_entry_t));
	    if ( ptr->next == NULL ) {
		out_err("Insufficient memory. Dying now");
		exit(1);
	    }
	    ptr->next->record = (hostrecord_t*) malloc(sizeof(hostrecord_t));
	    if ( ptr->next->record == NULL ) {
		out_err("Insufficient memory. Dying now");
		exit(1);
	    }

	    ptr->next->next = NULL;
	    memcpy(ptr->next->record, record[i], sizeof(hostrecord_t));
	    ptr = ptr->next;
	}

	/* LEAVE FUNCTION */
	goto ENDOK;
    }


    /* The queue is not empty, so get the last entry */
    while ( ptr->next != NULL)
	ptr=ptr->next;

    for (i=0; i<len; i++) {
	ptr->next = (queue_entry_t*) malloc(sizeof(queue_entry_t));
	if ( queue->head == NULL ) {
	    out_err("Insufficient memory. Dying now");
	    exit(1);
	}

	ptr->next->record = (hostrecord_t*) malloc(sizeof(hostrecord_t));
	if ( ptr->next->record == NULL ) {
	    out_err("Insufficient memory. Dying now");
	    exit(1);
	}

	ptr->next->next = NULL;
	memcpy(ptr->next->record, &record[i], sizeof(hostrecord_t));
	ptr = ptr->next;

    }

 ENDOK:
    retval = pthread_cond_signal(&(queue->cond));
    if ( retval != 0 )
	out_syserr(retval, "Client Worker [%i]: error signaling queue cond in client_queue_append", pthread_self());

    retval = pthread_mutex_unlock(&(queue->mutex));
    if ( retval != 0 )
	out_syserr(retval, "Client Worker [%i]: error locking queue mutex in client_queue_append", pthread_self());

    return RETVAL_OK;
}
