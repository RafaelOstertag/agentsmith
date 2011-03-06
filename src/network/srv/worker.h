
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

#ifndef NETWORK_WORKER_H
#define NETWORK_WORKER_H

struct _worker_thread_args {
    int       connfd;
    socklen_t addrlen;
    void     *addr;
};
typedef struct _worker_thread_args worker_thread_args_t;

extern void *network_server_worker(void *args);

#endif
