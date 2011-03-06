
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

#ifndef NETWORK_CLIENT_CLIENT_H
#define NETWORK_CLIENT_CLIENT_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "records.h"
#include "cfg.h"

extern int client_start(const addrinfo_list_t *aglist);
extern int client_queue_record(const hostrecord_t *hr);
extern int client_queue_flush();
extern int client_stop();

#endif
