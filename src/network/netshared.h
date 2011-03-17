
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

/**
 * Here is stuff that is shared between server and client.
 */

#ifndef NETWORK_NETSHARED_H
#define NETWORK_NETSHARED_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "records.h"

enum {
    ADD,
    REMOVE,			/* Not implemented */
    INFORMATION			/* Not implemented */
} RemoteAgentSmithCommands;

extern size_t net_command_to_buff(uint32_t command, const hostrecord_t *rec,
				  char *buff, size_t buffsize);
extern size_t net_buff_to_command(const char *buff, uint32_t * command,
				  hostrecord_t *rec);
extern ssize_t net_read(int fildes, void *buf, size_t nbyte, int *err);
extern ssize_t net_write(int fildes, const void *buf, size_t nbyte, int *err);

#ifdef WORDS_BIGENDIAN
#define htonll(x) (x)
#define ntohll(x) (x)
#else
extern uint64_t htonll(uint64_t a);
extern uint64_t ntohll(uint64_t a);
#endif

#ifdef DEBUG
extern void __dbg_dump_host_record(hostrecord_t *ptr);
#endif

#endif
