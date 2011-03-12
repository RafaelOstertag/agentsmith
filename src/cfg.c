
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
 * You should have received a copy of the GNU General Public License
 * along with agentsmith.  If not, see <http://www.gnu.org/licenses/>.
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
#include "output.h"
#include "cfg.h"

config_t  CONFIG;

static int config_initialized = 0;

/*
 * Default values if no values are specified
 */
#define DEFAULT_REGEX "Failed keyboard-interactive for [\\w ]+ from ([\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3})"
#define DEFAULT_LOGFILE "/var/log/authlog"
#define DEFAULT_ACTION "/bin/true"
#define DEFAULT_EXCLUDE ""
#define DEFAULT_SSL_CA_TRUST ""
#define DEFAULT_SSL_SERVER_CERT ""
#define DEFAULT_SSL_SERVER_KEY ""
#define DEFAULT_SSL_CLIENT_CERT ""
#define DEFAULT_SSL_CLIENT_KEY ""
#define DEFAULT_LISTEN_PORT "48621"
#define DEFAULT_LISTEN "0.0.0.0:48621"
#define DEFAULT_SERVER_BACKLOG 64
#define DEFAULT_INFORM_RETRY 30;
#define DEFAULT_INFORM_RETRY_WAIT 3
#define DEFAULT_ACTION_THRESHOLD 3
#define DEFAULT_TIME_INTERVAL 60
#define DEFAULT_PURGE_AFTER 3600
#define DEFAULT_SERVER 0
#define DEFAULT_SERVER_TIMEOUT 5
#define DEFAULT_INFORM 0
#define DEFAULT_MAXINCONNECTIONS 5

/**
 * The type of the values read from the files. Used by struct _cfgcfg
 */
enum cfgvaltype {
    CFG_VAL_STR = 0,
    CFG_VAL_PATH = 1,
    CFG_VAL_INT32 = 2,
    CFG_VAL_INT64 = 3,
    CFG_VAL_CALLBACK = 4
};

typedef int (*cfgcallback) (const char *);

/**
 * This struct will be used to specify the known options, value type, access
 * mode if applicable and the pointer where the value is stored.
 */
struct _cfgcfg {
    char     *name;
    enum cfgvaltype type;

	/**
     * This field is used if the type is CFG_VAL_PATH. It determines the
     * amode access() has to check for and is ignored otherwise.
     * 
     * A value of -1 indicates that no check is required even though
     * it is of type CFG_VAL_PATH.
     */
    int       amode;

	/**
	 * This points to the address where the value read from the
	 * config file will be stored. It must be of the proper type
	 * for 'type'
	 */
    void     *ptr;

	/**
	 * This will be used if the 'type' is CFG_VAL_CALLBACK. Using
	 * this, 'ptr' can be NULL.
	 */
    cfgcallback cb;
};

/*
 * The callback functions for CFG_VAL_CALLBACK are declared here.
 */
static int _config_callback_listen(const char *str);
static int _config_callback_inform(const char *str);

/**
 * The configuration options currently known. Please make sure the last entry
 * has *name set to NULL.
 */
struct _cfgcfg cfgcfg[] = {
    /*
     * The pid file may not exists at the time of reading the configuration
     * file, thus we specify -1 for amode.
     */
    {"pidfile", CFG_VAL_PATH, -1, (void *) &(CONFIG.pidfile), NULL},
    {"syslogfile", CFG_VAL_PATH, R_OK | F_OK, (void *) &(CONFIG.syslogfile),
     NULL},
    {"action", CFG_VAL_PATH, R_OK | F_OK | X_OK, (void *) &(CONFIG.action),
     NULL},
    {"exclude", CFG_VAL_PATH, R_OK | F_OK, (void *) &(CONFIG.exclude), NULL},
    {"regex", CFG_VAL_STR, -1, (void *) &(CONFIG.regex), NULL},
    {"action_threshold", CFG_VAL_INT32, -1,
     (void *) &(CONFIG.action_threshold), NULL},
    {"time_interval", CFG_VAL_INT64, -1, (void *) &(CONFIG.time_interval),
     NULL},
    {"purge_after", CFG_VAL_INT64, -1, (void *) &(CONFIG.purge_after), NULL},
    {"server", CFG_VAL_INT32, -1, (void *) &(CONFIG.server), NULL},
    {"server_timeout", CFG_VAL_INT32, -1, (void *) &(CONFIG.server_timeout),
     NULL},
    {"inform", CFG_VAL_INT32, -1, (void *) &(CONFIG.inform), NULL},
    {"inform_retry", CFG_VAL_INT32, -1, (void *) &(CONFIG.inform_retry),
     NULL},
    {"inform_retry_wait", CFG_VAL_INT32, -1,
     (void *) &(CONFIG.inform_retry_wait), NULL},
    {"maxinconnections", CFG_VAL_INT32, -1,
     (void *) &(CONFIG.maxinconnections), NULL},
    {"remote_authoritative", CFG_VAL_INT32, -1,
     (void *) &(CONFIG.remote_authoritative), NULL},
    {"listen", CFG_VAL_CALLBACK, -1, NULL, _config_callback_listen},
    {"inform_agent", CFG_VAL_CALLBACK, -1, NULL, _config_callback_inform},
    {"server_backlog", CFG_VAL_INT32, -1, (void *) &(CONFIG.server_backlog),
     NULL},
    {"ssl_ca_file", CFG_VAL_PATH, R_OK | F_OK,
     (void *) &(CONFIG.ssl_ca_file), NULL},
    {"ssl_server_cert", CFG_VAL_PATH, R_OK | F_OK,
     (void *) &(CONFIG.ssl_server_cert), NULL},
    {"ssl_server_key", CFG_VAL_PATH, R_OK | F_OK,
     (void *) &(CONFIG.ssl_server_key), NULL},
    {"ssl_client_cert", CFG_VAL_PATH, R_OK | F_OK,
     (void *) &(CONFIG.ssl_client_cert), NULL},
    {"ssl_client_key", CFG_VAL_PATH, R_OK | F_OK,
     (void *) &(CONFIG.ssl_client_key), NULL},
    {NULL, -1, -1, NULL, NULL}
};

static void
_init_config() {
    strncpy(CONFIG.pidfile, DEFAULT_PIDFILE, _MAX_PATH);
    strncpy(CONFIG.regex, DEFAULT_REGEX, BUFFSIZE);
    strncpy(CONFIG.syslogfile, DEFAULT_LOGFILE, _MAX_PATH);
    strncpy(CONFIG.action, DEFAULT_ACTION, _MAX_PATH);
    strncpy(CONFIG.exclude, DEFAULT_EXCLUDE, _MAX_PATH);
    strncpy(CONFIG.ssl_ca_file, DEFAULT_SSL_CA_TRUST, _MAX_PATH);
    strncpy(CONFIG.ssl_server_key, DEFAULT_SSL_SERVER_KEY, _MAX_PATH);
    strncpy(CONFIG.ssl_server_cert, DEFAULT_SSL_SERVER_CERT, _MAX_PATH);
    strncpy(CONFIG.ssl_client_key, DEFAULT_SSL_CLIENT_KEY, _MAX_PATH);
    strncpy(CONFIG.ssl_client_cert, DEFAULT_SSL_CLIENT_CERT, _MAX_PATH);

    CONFIG.action_threshold = DEFAULT_ACTION_THRESHOLD;
    CONFIG.time_interval = DEFAULT_TIME_INTERVAL;
    CONFIG.purge_after = DEFAULT_PURGE_AFTER;
    CONFIG.server = DEFAULT_SERVER;
    CONFIG.server_timeout = DEFAULT_SERVER_TIMEOUT;
    CONFIG.inform = DEFAULT_INFORM;
    CONFIG.inform_retry = DEFAULT_INFORM_RETRY;
    CONFIG.inform_retry_wait = DEFAULT_INFORM_RETRY_WAIT;
    CONFIG.maxinconnections = DEFAULT_MAXINCONNECTIONS;
    CONFIG.listen = NULL;
    CONFIG.inform_agents = NULL;
}

/**
 * This is the callback function that parses the 'listen' option's value
 */
static int
_config_callback_listen(const char *str) {
    char      host[BUFFSIZE];
    char      port[BUFFSIZE];
    char     *copy, *ptr, *colon;
    int       retval, number_of_listen = 0;
    struct addrinfo hints;
    struct addrinfo *tmp;
    addrinfo_list_t *newitem = NULL;
    addrinfo_list_t *lastitem = NULL;

    assert(CONFIG.listen == NULL);
    /*
     * Do some sanity checks
     */
    if (str == NULL)
	return RETVAL_ERR;

    /*
     * Prepare hints
     */
    memset(&hints, 0, sizeof (struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;

    copy = strdup(str);
    assert(copy != NULL);

    ptr = strtok(copy, " ");
    while (ptr != NULL && number_of_listen < MAX_LISTEN) {
	if (ptr[0] == '[') {
	    /*
	     * IPv6 Address
	     */

	    char     *closing;

	    closing = strchr(ptr, ']');
	    if (closing == NULL) {
		out_err("'%s' is not a valid IPv6 address", ptr);
		goto NEXT;
	    }

	    /*
	     * get port
	     */
	    colon = strchr(closing, ':');
	    if (colon == NULL) {
		strncpy(port, DEFAULT_LISTEN_PORT,
			strlen(DEFAULT_LISTEN_PORT) + 1);
	    } else {
		strncpy(port, colon + 1, strlen(colon + 1));
		port[strlen(colon + 1)] = '\0';
	    }

	    /*
	     * get host
	     */
	    if (colon != NULL) {
		strncpy(host, ptr + 1, (colon - ptr - 2));
		host[(colon - ptr - 2)] = '\0';
	    } else {
		strncpy(host, ptr + 1, strlen(ptr + 1));
		host[strlen(ptr + 1) - 1] = '\0';
	    }
	} else {
	    /*
	     * IPv4 or host name
	     */
	    colon = strchr(ptr, ':');

	    if (colon == NULL) {
		strncpy(host, ptr, strlen(ptr));
		host[strlen(ptr)] = '\0';
		strncpy(port, DEFAULT_LISTEN_PORT,
			strlen(DEFAULT_LISTEN_PORT) + 1);
	    } else {
		strncpy(host, ptr, colon - ptr);
		host[colon - ptr] = '\0';
		strncpy(port, colon + 1, strlen(colon + 1));
		port[strlen(colon + 1)] = '\0';
	    }
	}

	out_dbg("listen '%s' has been split to '%s' and '%s'", ptr, host,
		port);

	retval = getaddrinfo(host, port, &hints, &tmp);
	if (retval != 0) {
	    out_err
		("Unable to obtain information for listening on '%s' port '%s' (%s)",
		 host, port, gai_strerror(retval));
	    goto NEXT;
	}

	newitem = (addrinfo_list_t *) malloc(sizeof (addrinfo_list_t));
	if (newitem == NULL) {
	    out_err("Unable to allocate memory. Dying now");
	    exit(1);
	}
	newitem->addr = tmp;
	newitem->next = NULL;

	if (CONFIG.listen == NULL)
	    CONFIG.listen = newitem;
	else
	    lastitem->next = newitem;

	lastitem = newitem;

	number_of_listen++;

      NEXT:
	ptr = strtok(NULL, " ");
    }

    free(copy);

    return RETVAL_OK;
}

/**
 * This is the callback function that parses the 'inform' option's value
 */
static int
_config_callback_inform(const char *str) {
    char      host[BUFFSIZE];
    char      port[BUFFSIZE];
    char     *copy, *ptr, *colon;
    int       retval;
    struct addrinfo hints;
    struct addrinfo *tmp;
    addrinfo_list_t *new_agent = NULL;
    addrinfo_list_t *last_agent = NULL;

    assert(CONFIG.inform_agents == NULL);
    /*
     * Do some sanity checks
     */
    if (str == NULL)
	return RETVAL_ERR;

    /*
     * Prepare hints
     */
    memset(&hints, 0, sizeof (struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;

    copy = strdup(str);
    assert(copy != NULL);

    ptr = strtok(copy, " ");
    while (ptr != NULL) {
	if (ptr[0] == '[') {
	    /*
	     * IPv6 Address
	     */

	    char     *closing;

	    closing = strchr(ptr, ']');
	    if (closing == NULL) {
		out_err("'%s' is not a valid IPv6 address", ptr);
		goto NEXT;
	    }

	    /*
	     * get port
	     */
	    colon = strchr(closing, ':');
	    if (colon == NULL) {
		strncpy(port, DEFAULT_LISTEN_PORT,
			strlen(DEFAULT_LISTEN_PORT) + 1);
	    } else {
		strncpy(port, colon + 1, strlen(colon + 1));
		port[strlen(colon + 1)] = '\0';
	    }

	    /*
	     * get host
	     */
	    if (colon != NULL) {
		strncpy(host, ptr + 1, (colon - ptr - 2));
		host[(colon - ptr - 2)] = '\0';
	    } else {
		strncpy(host, ptr + 1, strlen(ptr + 1));
		host[strlen(ptr + 1) - 1] = '\0';
	    }
	} else {
	    /*
	     * IPv4 or host name
	     */
	    colon = strchr(ptr, ':');

	    if (colon == NULL) {
		strncpy(host, ptr, strlen(ptr));
		host[strlen(ptr)] = '\0';
		strncpy(port, DEFAULT_LISTEN_PORT,
			strlen(DEFAULT_LISTEN_PORT) + 1);
	    } else {
		strncpy(host, ptr, colon - ptr);
		host[colon - ptr] = '\0';
		strncpy(port, colon + 1, strlen(colon + 1));
		port[strlen(colon + 1)] = '\0';
	    }
	}

	out_dbg("inform_agent '%s' has been split to '%s' and '%s'", ptr,
		host, port);

	retval = getaddrinfo(host, port, &hints, &tmp);
	if (retval != 0) {
	    out_err
		("Unable to obtain information for host '%s' port '%s' (%s)",
		 host, port, gai_strerror(retval));
	    goto NEXT;
	}

	new_agent = (addrinfo_list_t *) malloc(sizeof (addrinfo_list_t));
	if (new_agent == NULL) {
	    out_err("Unable to allocate memory. Dying now");
	    exit(1);
	}
	new_agent->addr = tmp;
	new_agent->next = NULL;

	if (CONFIG.inform_agents == NULL)
	    CONFIG.inform_agents = new_agent;
	else
	    last_agent->next = new_agent;

	last_agent = new_agent;
      NEXT:
	ptr = strtok(NULL, " ");
    }

    free(copy);

    return RETVAL_OK;
}

/**
 * Sanitize configuration options if necessary.
 */
void
_config_sanitize() {
    CONFIG.server_backlog =
	CONFIG.server_backlog <
	1 ? DEFAULT_SERVER_BACKLOG : CONFIG.server_backlog;

    CONFIG.maxinconnections =
	CONFIG.maxinconnections >
	MAX_INCONN ? MAX_INCONN : CONFIG.maxinconnections;

    CONFIG.maxinconnections =
	CONFIG.maxinconnections <
	1 ? DEFAULT_MAXINCONNECTIONS : CONFIG.maxinconnections;

    CONFIG.server_timeout =
	CONFIG.server_timeout <
	1 ? DEFAULT_SERVER_TIMEOUT : CONFIG.server_timeout;

    CONFIG.inform_retry_wait =
	CONFIG.inform_retry_wait <
	1 ? DEFAULT_INFORM_RETRY_WAIT : CONFIG.inform_retry_wait;

    CONFIG.inform = CONFIG.inform != 0 ? 1 : 0;

    CONFIG.server = CONFIG.server != 0 ? 1 : 0;

    CONFIG.action_threshold =
	CONFIG.action_threshold <
	1 ? DEFAULT_ACTION_THRESHOLD : CONFIG.action_threshold;

    CONFIG.time_interval =
	CONFIG.time_interval <
	1 ? DEFAULT_TIME_INTERVAL : CONFIG.time_interval;

    CONFIG.purge_after =
	CONFIG.purge_after < 1 ? DEFAULT_PURGE_AFTER : CONFIG.purge_after;
}

/**
 * Places copy the given value to the location as specified in cfgcfg.
 */
static void
_set_config_option(const char *token, const char *value) {
    struct _cfgcfg *ptr;
    int       known_option = 0;

    assert(token != NULL && value != NULL);

    ptr = cfgcfg;
    while (ptr->name != NULL) {
	if (strcmp(ptr->name, token) == 0) {
	    switch (ptr->type) {
	    case CFG_VAL_STR:
		strncpy((char *) ptr->ptr, value, BUFFSIZE);
		break;
	    case CFG_VAL_PATH:
		strncpy((char *) ptr->ptr, value, _MAX_PATH);
		break;
	    case CFG_VAL_INT32:
		sscanf(value, "%i", (int32_t *) ptr->ptr);
		break;
	    case CFG_VAL_INT64:
		if (sizeof (long) == 8)
		    sscanf(value, "%li", (int64_t *) ptr->ptr);
		else
		    sscanf(value, "%lli", (int64_t *) ptr->ptr);
		break;
	    case CFG_VAL_CALLBACK:
		ptr->cb(value);
		break;
	    default:
		out_err("%i is not a known configuration type\n", ptr->type);
		abort();
	    }
	    known_option = 1;
	}
	ptr++;
    }
    if (!known_option) {
	out_err("'%s' is not a known option", token);
    }

    _config_sanitize();

}

/**
 * Error message if we cannot access a file.
 */
static void
_file_access_error(int err, const char *filepath) {
    out_syserr(err, "Error accessing '%s'", filepath);
    exit(1);
}

/**
 * Checks all configuration options of the type CFG_TYPE_PATH for proper
 * access.
 */
static void
_check_paths_in_config() {
    struct _cfgcfg *ptr;

    ptr = cfgcfg;
    while (ptr->name != NULL) {
	if (ptr->type == CFG_VAL_PATH &&
	    ptr->amode != -1 && strlen(ptr->ptr) > 0) {
	    int       retval;
	    retval = access((char *) ptr->ptr, ptr->amode);
	    if (retval == -1) {
		_file_access_error(errno, (char *) ptr->ptr);
	    }
	}
	ptr++;
    }
}

/**
 * Read the config file. Returns a pointer to the CONFIG struct.
 */
int
config_read(const char *file) {
    int       retval;
    FILE     *f;
    char      line[BUFFSIZE], token[BUFFSIZE], value[BUFFSIZE];
    char     *pos, *tmp;

    assert(file != NULL);
    retval = access(file, F_OK | R_OK);
    if (retval == -1) {
	_file_access_error(errno, file);
    }

    _init_config();

    f = fopen(file, "r");
    if (f == NULL) {
	out_err("Unable to open %s", file);
	exit(1);
    }

    /*
     * Read the configuration file line by line
     */
    while (!feof(f)) {
	fgets(line, BUFFSIZE, f);
	if (strlen(line) < 2)
	    continue;

	if (line[0] == '#')
	    continue;

	tmp = line;
#ifdef HAVE_STRSEP
	pos = strsep(&tmp, "=");
#else
	/*
	 * On e.g., solaris 10 strsep() is not available, so we emulate
	 * it...
	 */
	pos = strchr(tmp, '=');
	if (pos != NULL) {
	    tmp[pos - tmp] = '\0';
	    if (pos + 1 != '\0') {
		tmp = pos + 1;
	    } else {
		tmp = NULL;
	    }
	    pos = line;
	}
#endif
	if (pos != NULL) {
	    strncpy(token, pos, BUFFSIZE);
	    if (tmp == NULL || strlen(tmp) == 0)
		continue;
	    strncpy(value, tmp, BUFFSIZE);
	    tmp = strchr(value, '\n');
	    if (tmp != NULL)
		*tmp = '\0';
	    /*
	     * Maybe the value is now empty, i.e. the configuration file has
	     * line like:
	     *
	     * blabla=
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

    /*
     * If there are no listens in the config file, we add the default
     */
    if (CONFIG.listen == NULL)
	_config_callback_listen(DEFAULT_LISTEN);

    out_dbg("pidfile=%s", CONFIG.pidfile);
    out_dbg("syslogfile=%s", CONFIG.syslogfile);
    out_dbg("action=%s", CONFIG.action);
    out_dbg("exclude=%s", CONFIG.exclude);
    out_dbg("regex=%s", CONFIG.regex);
    out_dbg("action_threshold=%i", CONFIG.action_threshold);
    if (sizeof (long) == 8)
	out_dbg("time_interval=%li", CONFIG.time_interval);
    else
	out_dbg("time_interval=%lli", CONFIG.time_interval);

    if (sizeof (long) == 8)
	out_dbg("purge_after=%li", CONFIG.purge_after);
    else
	out_dbg("purge_after=%lli", CONFIG.purge_after);

    out_dbg("server=%i", CONFIG.server);
    out_dbg("server_timeout=%i", CONFIG.server_timeout);
    out_dbg("server_backlog=%i", CONFIG.server_backlog);
    out_dbg("maxinconnections=%i", CONFIG.maxinconnections);
    out_dbg("inform=%i", CONFIG.inform);
    out_dbg("inform_retry=%i", CONFIG.inform_retry);
    out_dbg("inform_retry_wait=%i", CONFIG.inform_retry_wait);
    out_dbg("remote_authoritative=%i", CONFIG.remote_authoritative);
    out_dbg("ssl_ca_file=%s", CONFIG.ssl_ca_file);
    out_dbg("ssl_server_cert=%s", CONFIG.ssl_server_cert);
    out_dbg("ssl_server_key=%s", CONFIG.ssl_server_key);
    out_dbg("ssl_client_cert=%s", CONFIG.ssl_client_cert);
    out_dbg("ssl_client_key=%s", CONFIG.ssl_client_key);

    if (CONFIG.remote_authoritative)
	out_msg
	    ("Exclude file will be ignored when receiving hosts from remote clients");

    config_initialized = 1;
    return RETVAL_OK;
}

int
config_free() {
    addrinfo_list_t *ptr, *nextptr;

    if (!config_initialized)
	return RETVAL_ERR;

    ptr = CONFIG.listen;
    nextptr = NULL;
    while (ptr != NULL) {
	freeaddrinfo(ptr->addr);
	nextptr = ptr->next;
	free(ptr);
	ptr = nextptr;
    }

    ptr = CONFIG.inform_agents;
    nextptr = NULL;
    while (ptr != NULL) {
	freeaddrinfo(ptr->addr);
	nextptr = ptr->next;
	free(ptr);
	ptr = nextptr;
    }
    config_initialized = 0;

    return RETVAL_OK;
}
