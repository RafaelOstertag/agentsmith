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

#ifndef CFG_H
#define CFG_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#include "globals.h"

enum {
    /*
     * Maximum number of listening sockets 
     */
    MAX_LISTEN = 256,
    /*
     * Maximum number of inbound connections (threads) 
     */
    MAX_INCONN = 256,
};

struct _addrinfo_list {
    struct addrinfo *addr;
    struct _addrinfo_list *next;
};
typedef struct _addrinfo_list addrinfo_list_t;

struct _config {
    char      pidfile[_MAX_PATH];
    char      syslogfile[_MAX_PATH];
    char      action[_MAX_PATH];
    char      exclude[_MAX_PATH];
    char      ssl_ca_trust[_MAX_PATH];
    char      regex[BUFFSIZE];
    int32_t   action_threshold;
    int64_t   time_interval;
    int64_t   purge_after;
    int32_t   server;
    int32_t   server_timeout;
    int32_t   server_backlog;
    int32_t   maxinconnections;
    int32_t   inform;
    int32_t   inform_retry;
    int32_t   inform_retry_wait;
    int32_t   remote_authoritative;
    addrinfo_list_t *listen;
    /*
     * This is the start point for the linked lists of daemons to inform
     */
    addrinfo_list_t *inform_agents;
};
typedef struct _config config_t;

extern config_t CONFIG;

extern int config_read(const char *file);
extern int config_free();

#endif
