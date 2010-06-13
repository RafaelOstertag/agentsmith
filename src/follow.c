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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef STDC_HEADERS
#include <stdlib.h>
#endif

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include "globals.h"
#include "follow.h"
#include "output.h"
#include "regex.h"

static char BUFF[BUFFSIZE];
static int stop = 0; /* Will be set by stop_following to 1 in order to stop */
static const int sleep_time = 250000;

static void
readtoeof (FILE *file) {
    static int buffpos = 0;
    int c;

    while (!stop) {
	c = fgetc(file);
	if ( feof (file) ) {
	    clearerr(file);
	    break;
	}
	if ( ferror (file) ) {
	    out_syserr(errno, "File error");
	    exit (1);
	}

	BUFF[buffpos] = (unsigned char)c;
	buffpos++;
	if (buffpos == BUFFSIZE-1) {
	    BUFF[buffpos] = '\0';
	    buffpos=0;
	    regex_do(BUFF);
	    continue;
	}
	if ( c == '\n' ) {
	    BUFF[buffpos] = '\0';
	    buffpos=0;
	    regex_do(BUFF);
	}
    }
}

void
follow (const char* fname) {
    struct stat sb;
    FILE *file;
    off_t curpos, lastpos;
    ino_t curino, lastino;
    int retval;

    file = fopen(fname, "r");
    if (file == NULL) {
	out_syserr(errno, "Unable to open '%s'", fname);
	exit (1);
    }

    retval = stat ( fname, &sb);
    if (retval == -1) {
	out_syserr(errno, "Unable to stat '%s'", fname);
	exit (1);
    }
    lastpos=curpos=sb.st_size;
    lastino=curino=sb.st_ino;
    retval = fseek(file, curpos, SEEK_SET);
    if (retval == -1) {
	out_syserr(errno, "Unable to seek '%s'", fname);
	exit (1);
    }

    while (!stop) {
	retval = stat(fname, &sb);
	if (retval == -1) {
	    /* The file is gone... */
	    fclose ( file );
	    file = NULL;
	    usleep(sleep_time);
	    continue;
	} else {
	    /* Maybe the file has reappeared */
	    if ( file == NULL ) {
		/* Ok, the file really reappeared */
		file = fopen ( fname, "r" );
		if ( file == NULL) {
		    out_syserr(errno, "Unable to re-open '%s'", fname);
		    exit (1);
		}
	    } else {
		/* Maybe the inode changed? */
		if ( lastino != curino ) {
		    file = freopen( fname, "r", file);
		    if ( file == NULL ) {
			out_syserr(errno, "Unable to re-open '%s'", fname);
			exit (1);
		    }
		    lastino = curino;
		}
	    }
	}
	curpos=sb.st_size;
	if ( curpos > lastpos ) {
	    readtoeof(file);
	    lastpos=curpos;
	} else if ( lastpos > curpos ) {
	    rewind(file);
	    clearerr(file);
	    readtoeof(file);
	    lastpos=curpos;
	}
	usleep(sleep_time);
    }
    stop = 0;
}

void
follow_stop() {
    stop = 1;
}
