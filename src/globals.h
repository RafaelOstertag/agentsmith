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

#ifndef GLOBALS_H
#define GLOBALS_H

#define LOCALHOST "localhost"

enum {
    _MAX_PATH = 1024,
    BUFFSIZE = 1024,
    RETVAL_OK = 0,
    RETVAL_ERR = -1,
    /*
     * Maximum size of a sockaddr struct len. This is just an arbitrary value
     * big enough to hold IPv4 and IPv6 structs. 
     */
    MYSOCKADDRLEN = 64,
    /*
     * Should be sufficient even for IPv6 
     */
    IPADDR_SIZE = 46,
    /*
     * This is the size of the command send over the wire. It is computed by
     * taken the size of hostrecord_t plus 4 bytes for command 
     */
    REMOTE_COMMAND_SIZE = 144
};

#endif
