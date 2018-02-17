
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef STDC_HEADERS
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#endif

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#include "output.h"

enum {
    BUFFSIZE = 1024
};

static outtype_t OTYPE = CONSOLE;
static int SYSLOG_INIT = 0;

void
out_done() {
    if (SYSLOG_INIT) {
	closelog();
	SYSLOG_INIT = 0;
    }
}

void
out_settype(outtype_t type) {
    switch (type) {
    case SYSLOG:
	if (!SYSLOG_INIT) {
	    openlog(PACKAGE_NAME, LOG_PID, LOG_DAEMON);
	    SYSLOG_INIT = 1;
	}
	break;
    case CONSOLE:
	if (SYSLOG_INIT) {
	    closelog();
	    SYSLOG_INIT = 0;
	}
	break;
    }
    OTYPE = type;
}

outtype_t
out_gettype() {
    return OTYPE;
}

void
out_err(const char *format, ...) {
    char      buff[BUFFSIZE];
#ifdef HAVE_SYSLOG_R
     struct syslog_data data = SYSLOG_DATA_INIT;
#endif

    va_list   ap;
    va_start(ap, format);
    vsnprintf(buff, BUFFSIZE, format, ap);
    va_end(ap);

    switch (OTYPE) {
        case CONSOLE:
            fprintf(stderr, "%s\n", buff);
            break;
        case SYSLOG:
#ifdef HAVE_SYSLOG_R
            syslog_r(LOG_ERR, &data, "%s", buff);
#else
            syslog(LOG_ERR, "%s", buff);
#endif
	break;
    }
    va_end(ap);
}

void
out_syserr(int no, const char *format, ...) {
    char      buff[BUFFSIZE];
    char      buff2[BUFFSIZE];
#ifdef HAVE_SYSLOG_R
     struct syslog_data data = SYSLOG_DATA_INIT;
#endif
    va_list   ap;

#ifdef HAVE_STRERROR_R
#ifdef STRERROR_R_CHAR_P
    char      buff3[BUFFSIZE];
    char     *strerr;
    strerr = strerror_r(no, buff3, BUFFSIZE);
#else
    char      strerr[BUFFSIZE];
    strerror_r(no, strerr, BUFFSIZE);
#endif
#elif HAVE_STRERROR
    char     *strerr;
    strerr = strerror(no);
#else
#error "Neither strerror_r nor strerror available"
#endif

    va_start(ap, format);
    vsnprintf(buff, BUFFSIZE, format, ap);
    va_end(ap);

    snprintf(buff2, BUFFSIZE, "%s (errval %i: %s)", buff, no, strerr);

    switch (OTYPE) {
    case CONSOLE:
	fprintf(stderr, "%s\n", buff2);
	break;
    case SYSLOG:
#ifdef HAVE_SYSLOG_R
        syslog_r(LOG_ERR, &data, "%s", buff2);
#else
	syslog(LOG_ERR, "%s", buff2);
#endif
	break;
    }
}

void
out_msg(const char *format, ...) {
    char      buff[BUFFSIZE];
#ifdef HAVE_SYSLOG_R
     struct syslog_data data = SYSLOG_DATA_INIT;
#endif

    va_list   ap;
    va_start(ap, format);
    vsnprintf(buff, BUFFSIZE, format, ap);
    va_end(ap);

    switch (OTYPE) {
    case CONSOLE:
	fprintf(stdout, "%s\n", buff);
	break;
    case SYSLOG:
#ifdef HAVE_SYSLOG_R
        syslog_r(LOG_INFO, &data, "%s", buff);
#else
	syslog(LOG_INFO, "%s", buff);
#endif
	break;
    }
}

#ifdef DEBUG
void
out_dbg(const char *format, ...) {
#ifdef HAVE_SYSLOG_R
     struct syslog_data data = SYSLOG_DATA_INIT;
#endif
    char      buff[BUFFSIZE];

    va_list   ap;
    va_start(ap, format);
    vsnprintf(buff, BUFFSIZE, format, ap);
    va_end(ap);

    switch (OTYPE) {
    case CONSOLE:
	fprintf(stdout, "DEBUG: %s\n", buff);
	break;
    case SYSLOG:
#ifdef HAVE_SYSLOG_R
        syslog_r(LOG_DEBUG, &data, "%s", buff);
#else
	syslog(LOG_DEBUG, "%s", buff);
#endif
	break;
    }
}
#endif
