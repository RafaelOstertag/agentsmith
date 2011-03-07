
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
static int stop = 0;		/* Will be set by stop_following to 1 in order to stop */

/* This will be fed to usleep() [usec] */
static const int sleep_time = 250000;

/* This will be fed to sleep() [sec] */
static const int err_sleep_time = 30;

static void
readtoeof(FILE * file) {
    static int buffpos = 0;
    int       c;

    while (!stop) {
	c = fgetc(file);
	if (feof(file)) {
	    clearerr(file);
	    break;
	}
	if (ferror(file)) {
	    out_syserr(errno, "File error. Sleeping for %i seconds",
		       err_sleep_time);
	    sleep(err_sleep_time);
	    return;
	}

	BUFF[buffpos] = (unsigned char) c;
	buffpos++;
	if (buffpos == BUFFSIZE - 1) {
	    /*
	     * Buffer is full, terminate and check 
	     */
	    BUFF[buffpos] = '\0';
	    buffpos = 0;
	    /*
	     * This will check if we have a regex match
	     */
	    regex_do(BUFF);
	    continue;
	}
	if (c == '\n') {
	    /*
	     * A new line, now check 
	     */
	    BUFF[buffpos] = '\0';
	    buffpos = 0;
	    /*
	     * This will check if we have a regex match
	     */
	    regex_do(BUFF);
	}
    }
}

void
follow(const char *fname) {
    struct stat sb;
    FILE     *file;
    off_t     curpos, lastpos;
    ino_t     curino, lastino;
    int       retval;

    file = fopen(fname, "r");
    if (file == NULL) {
	out_syserr(errno, "Unable to open '%s'", fname);
	exit(1);
    }

    retval = stat(fname, &sb);
    if (retval == -1) {
	out_syserr(errno, "Unable to stat '%s'", fname);
	exit(1);
    }
    lastpos = curpos = sb.st_size;
    lastino = curino = sb.st_ino;
    retval = fseek(file, curpos, SEEK_SET);
    if (retval == -1) {
	out_syserr(errno, "Unable to seek '%s'", fname);
	exit(1);
    }

    while (!stop) {
	retval = stat(fname, &sb);
	if (retval == -1) {
	    /*
	     * The file is gone... 
	     */

	    /*
	     * This check is neccessary because we might end up several times
	     * here before the file reappears 
	     */
	    if (file != NULL) {
		fclose(file);
		file = NULL;
	    }
	    out_err("The file '%s' has gone. Going to sleep for %i seconds",
		    fname, err_sleep_time);
	    sleep(err_sleep_time);
	    continue;
	} else {
	    /*
	     * Maybe the file has reappeared 
	     */
	    if (file == NULL) {
		/*
		 * Ok, the file really reappeared 
		 */
		out_msg("'%s' has reappeared.", fname);
		file = fopen(fname, "r");
		if (file == NULL) {
		    out_syserr(errno,
			       "Unable to re-open '%s'. Going to sleep for %i seconds",
			       fname, err_sleep_time);
		    sleep(err_sleep_time);
		    continue;
		}
	    } else {
		/*
		 * Maybe the inode changed? 
		 */
		if (lastino != curino) {
		    out_msg("inode changed from %i to %i on '%s'.",
			    lastino, curino, fname);
		    file = freopen(fname, "r", file);
		    if (file == NULL) {
			out_syserr(errno,
				   "Unable to re-open '%s'. Going to sleep for %i seconds",
				   fname, err_sleep_time);
			sleep(err_sleep_time);
			continue;
		    }
		    lastino = curino;
		}
	    }
	}
	curpos = sb.st_size;
	if (curpos > lastpos) {
	    /*
	     * There is new data in the file, read 
	     */
	    readtoeof(file);
	    lastpos = curpos;
	} else if (lastpos > curpos) {
	    /*
	     * The file shrunk 
	     */
	    out_msg("'%s' shrunk from %i to %i bytes", fname, lastpos,
		    curpos);
	    file = freopen(fname, "r", file);
	    if (file == NULL) {
		out_syserr(errno,
			   "Unable to re-open '%s'. Going to sleep for %i seconds.",
			   fname, err_sleep_time);
		sleep(err_sleep_time);
		continue;
	    }
	    rewind(file);
	    clearerr(file);
	    /*
	     * Reset the lastpos, since the file shrunk 
	     */
	    lastpos = 0;
	    readtoeof(file);
	    lastpos = curpos;
	}
	usleep(sleep_time);
    }
    stop = 0;
}

void
follow_stop() {
    stop = 1;
}
