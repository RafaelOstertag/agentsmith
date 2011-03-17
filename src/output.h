
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

#ifndef OUTPUT_H
#define OUTPUT_H

enum _outtype {
    SYSLOG = 0,
    CONSOLE = 1
};

typedef enum _outtype outtype_t;

/* Call this function upon exit of the program. It will cleanup stuff is SYSLOG is used, or does nothing if CONSOLE is used. */
extern void out_done();
extern void out_settype(outtype_t type);
extern outtype_t out_gettype();
extern void out_err(const char *format, ...);
extern void out_syserr(int no, const char *format, ...);
extern void out_msg(const char *format, ...);
#ifdef DEBUG
extern void out_dbg(const char *format, ...);
#else
#define out_dbg(...)
#endif
#endif
