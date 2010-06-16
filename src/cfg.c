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

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include "globals.h"
#include "cfg.h"
#include "output.h"

/* The global config struct holding the configuration we read */
config CONFIG;

static int config_initialized = 0;
static const char DEFAULT_REGEX[] = "Failed keyboard-interactive for [\\w ]+ from ([\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3})";
static const char DEFAULT_LOGFILE[] = "/var/log/authlog";
static const char DEFAULT_ACTION[] = "/bin/true";

enum cfgvaltype {
    CFG_VAL_STR = 0,
    CFG_VAL_PATH = 1,
    CFG_VAL_INT = 2
};

struct _cfgcfg {
	char *name;
	enum cfgvaltype type;
	/* This field is used if the type is CFG_VAL_PATH. It determines the
	 * amode access() has to check for and is ignored otherwise.
	 *
	 * A value of -1 indicates that no check is required eventhough it is
	 * of type CFG_VAL_PATH.
	 */
	int amode;
	void *ptr;
};

struct _cfgcfg cfgcfg[] = {
    /* The pid file may not exists at the time of reading the configuration
       file, thus we specify -1 for amode. */
    { "pidfile", CFG_VAL_PATH, -1 , (void*) &(CONFIG.pidfile) },
    { "syslogfile", CFG_VAL_PATH, R_OK | F_OK, (void*) &(CONFIG.syslogfile) },
    { "action", CFG_VAL_PATH, R_OK | F_OK | X_OK, (void*) &(CONFIG.action) },
    { "regex", CFG_VAL_STR, -1, (void*)&(CONFIG.regex) },
    { "action_threshold", CFG_VAL_INT, -1, (void*) &(CONFIG.action_threshold) },
    { "time_interval", CFG_VAL_INT, -1, (void*) &(CONFIG.time_interval) },
    { "purge_after", CFG_VAL_INT, -1, (void*) &(CONFIG.purge_after) },
    { NULL, -1, -1, NULL }
};

static void
_init_config() {
    strncpy(CONFIG.pidfile, DEFAULT_PIDFILE, _MAX_PATH);
    strncpy(CONFIG.regex, DEFAULT_REGEX, BUFFSIZE);
    strncpy(CONFIG.syslogfile, DEFAULT_LOGFILE, _MAX_PATH);
    strncpy(CONFIG.action, DEFAULT_ACTION, _MAX_PATH);
    CONFIG.action_threshold = 3;
    CONFIG.time_interval = 60;
    CONFIG.purge_after = 3600;
}

static void
_set_config_option(const char* token, const char* value) {
    struct _cfgcfg *ptr;
    int known_option = 0;

    assert(token != NULL && value != NULL);

    ptr = cfgcfg;
    while ( ptr->name != NULL ) {
	if (strcmp(ptr->name, token) == 0 ) {
	    switch (ptr->type) {
	    case CFG_VAL_STR:
		strncpy((char*) ptr->ptr, value, BUFFSIZE);
		break;
	    case CFG_VAL_PATH:
		strncpy((char*) ptr->ptr, value, _MAX_PATH);
		break;
	    case CFG_VAL_INT:
		sscanf(value, "%i", (int*)ptr->ptr);
		break;
	    default:
		out_err("%i is not a known configuration type\n", ptr->type);
		abort();
	    }
	    known_option=1;
	}
	ptr++;
    }
    if (!known_option) {
	out_err("'%s' is not a known option", token);
    }
	
}

static void
_file_access_error(int err, const char* filepath) {
    out_syserr(err, "Error accessing '%s'", filepath);
    exit(1);
}

/*
 * Checks all configuration options of the type CFG_TYPE_PATH.
 */
static void
_check_paths_in_config() {
    struct _cfgcfg *ptr;

    ptr = cfgcfg;
    while ( ptr->name != NULL ) {
	if ( ptr->type == CFG_VAL_PATH && ptr->amode != -1 ) {
	    int retval;
	    retval = access ((char*)ptr->ptr, ptr->amode);
	    if ( retval == -1 ) {
		_file_access_error(errno, (char*)ptr->ptr);
	    }
	}
	ptr++;
    }
}

config*
config_read(const char* file) {
    int retval;
    FILE *f;
    char line[BUFFSIZE], token[BUFFSIZE], value[BUFFSIZE];
    char *pos, *tmp;

    assert( file != NULL );
    retval = access(file, F_OK | R_OK);
    if ( retval == -1 ) {
	_file_access_error(errno, file);
    }

    _init_config();

    f = fopen(file, "r");
    if ( f == NULL ) {
	out_err("Unable to open %s", file);
	exit (1);
    }

    /* Read the configuration file line by line */
    while ( !feof(f) ) {
	fgets(line, BUFFSIZE, f);
	if ( strlen(line) < 2 )
	    continue;

	if ( line[0] == '#' )
	    continue;

	tmp = line;
#ifdef HAVE_STRSEP
	pos = strsep(&tmp, "=");
#else
	/* On e.g., solaris 10 strsep() is not available, so we try to simulate
	   it... */
	pos = strchr(tmp, '=');
	if ( pos != NULL ) {
	    tmp[pos-tmp]='\0';
	    if ( pos+1 != '\0' ) {
		tmp = pos+1;
	    } else {
		tmp = NULL;
	    }
	    pos=line;
	}
#endif
	if ( pos != NULL ) {
	    strncpy ( token, pos, BUFFSIZE );
	    if ( tmp == NULL || strlen ( tmp ) == 0 )
		continue;
	    strncpy ( value, tmp, BUFFSIZE );
	    tmp = strchr(value, '\n');
	    if ( tmp != NULL )
		*tmp = '\0';
	    /* Maybe the value is now empty, i.e. the configuration file has
	       line like:

	       blabla=
	    */
	    if (strlen(value) == 0) {
		out_msg("configuration option %s has no value", token);
		continue;
	    }
	    _set_config_option(token, value);
	}
    }

    _check_paths_in_config();

    fclose(f);

    out_dbg("pidfile=%s",CONFIG.pidfile);
    out_dbg("syslogfile=%s",CONFIG.syslogfile);
    out_dbg("action=%s",CONFIG.action);
    out_dbg("regex=%s",CONFIG.regex);
    out_dbg("action_threshold=%i",CONFIG.action_threshold);
    out_dbg("time_interval=%i",CONFIG.time_interval);
    out_dbg("purge_after=%i",CONFIG.purge_after);
    
    config_initialized = 1;
    return &CONFIG;
}

config*
config_get() {
    if (!config_initialized)
	return NULL;

    return &CONFIG;
}
