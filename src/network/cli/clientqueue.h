
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

#ifndef NETWORK_CLIENT_CLIENTQUEUE_H
#define NETWORK_CLIENT_CLIENTQUEUE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#ifdef HAVE_SEMAPHORE_H
#include <semaphore.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "records.h"

struct _queue_entry {
    hostrecord_t *record;
    struct _queue_entry *next;
};
typedef struct _queue_entry queue_entry_t;

struct _client_queue {
    /*
     * This is the thread id of the associated thread 
     */
    pthread_t tid;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int       family;
    socklen_t socklen;
    struct sockaddr *sockaddr;

    queue_entry_t *head;
};
typedef struct _client_queue client_queue_t;

extern client_queue_t *client_queue_init(int af, const struct sockaddr *s,
					 socklen_t sl);
extern int client_queue_destroy(client_queue_t *queue);
extern int client_queue_append(client_queue_t *queue,
			       const hostrecord_t **record, int len);

#endif
