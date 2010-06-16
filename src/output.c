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

static OUTTYPE OTYPE = CONSOLE;
static int SYSLOG_INIT = 0;

void
out_done() {
    if ( SYSLOG_INIT ) {
	closelog();
	SYSLOG_INIT = 0;
    }
}

void
out_settype(OUTTYPE type) {
    switch (type) {
    case SYSLOG:
	if ( !SYSLOG_INIT ) {
	    openlog (PACKAGE_NAME, LOG_PID, LOG_DAEMON);
	    SYSLOG_INIT = 1;
	}
	break;
    case CONSOLE:
	if ( SYSLOG_INIT ) {
	    closelog();
	    SYSLOG_INIT = 0;
	}
	break;
    }
    OTYPE = type;
}

OUTTYPE
out_gettype() {
    return OTYPE;
}

void
out_err(const char *format, ...) {
    char buff[BUFFSIZE];

    va_list ap;
    va_start(ap, format);
    vsnprintf(buff, BUFFSIZE, format, ap);
    va_end(ap);

    switch (OTYPE) {
    case CONSOLE:
	fprintf(stderr, "%s\n", buff);
	break;
    case SYSLOG:
	syslog(LOG_ERR, buff);
	break;
    }
}

void
out_syserr(int no, const char *format, ...) {
    char buff[BUFFSIZE];
    char buff2[BUFFSIZE];
    va_list ap;

#ifdef HAVE_STRERROR_R
#ifdef DEBUG
#warning "Using strerror_r()"
#endif
    char strerr[BUFFSIZE];
    strerror_r(no, strerr, BUFFSIZE);
#elif HAVE_STRERROR
#ifdef DEBUG
#warning "Using strerror()"
#endif
    char *strerr;
    strerr = strerror(no);
#else
#error "Neither strerror_r nor strerror available"
#endif

    va_start(ap, format);
    vsnprintf(buff, BUFFSIZE, format, ap);
    va_end(ap);

    snprintf(buff2, BUFFSIZE, "%s (%s)", buff, strerr);

    switch (OTYPE) {
    case CONSOLE:
	fprintf(stderr, "%s\n", buff2);
	break;
    case SYSLOG:
	syslog(LOG_ERR, buff2);
	break;
    }
}

void
out_msg(const char *format, ...) {
    char buff[BUFFSIZE];

    va_list ap;
    va_start(ap, format);
    vsnprintf(buff, BUFFSIZE, format, ap);
    va_end(ap);

    switch (OTYPE) {
    case CONSOLE:
	fprintf(stdout, "%s\n", buff);
	break;
    case SYSLOG:
	syslog(LOG_INFO, buff);
	break;
    }
}

#ifdef DEBUG
void
out_dbg(const char *format, ...) {

    char buff[BUFFSIZE];

    va_list ap;
    va_start(ap, format);
    vsnprintf(buff, BUFFSIZE, format, ap);
    va_end(ap);

    switch (OTYPE) {
    case CONSOLE:
	fprintf(stdout, "DEBUG: %s\n", buff);
	break;
    case SYSLOG:
	syslog(LOG_DEBUG, buff);
	break;
    }
}
#endif
