/* Copyright (C) 2010 Rafael Ostertag 
 *
 * This file is part of sshsentinel.
 *
 * sshsentinel is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Foobar is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */

/* $Id$ */

#ifndef OUTPUT_H
#define OUTPUT_H

enum _OUTTYPE {
    SYSLOG = 0,
    CONSOLE = 1
};

typedef enum _OUTTYPE OUTTYPE;

/* Call this function upon exit of the program. It will cleanup stuff is SYSLOG
   is used, or does nothing if CONSOLE is used. */
extern void out_done();
extern void out_settype(OUTTYPE type);
extern OUTTYPE out_gettype();
extern void out_err(const char *format, ...);
extern void out_syserr(int no, const char *format, ...);
extern void out_msg(const char *format, ...);
#ifdef DEBUG
extern void out_dbg(const char *format, ...);
#else
#define out_dbg(...)
#endif
#endif
