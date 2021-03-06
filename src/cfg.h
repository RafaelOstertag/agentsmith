
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
    MAX_INCONN = 256
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
    char      ssl_ca_file[_MAX_PATH];
    char ssl_crl_file[_MAX_PATH];
    char      ssl_server_key[_MAX_PATH];
    char      ssl_server_cert[_MAX_PATH];
    char      ssl_client_key[_MAX_PATH];
    char      ssl_client_cert[_MAX_PATH];
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
