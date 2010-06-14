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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef STDC_HEADERS
#include <string.h>
#include <stdlib.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif

#include "cfg.h"
#include "output.h"
#include "follow.h"
#include "sighandler.h"
#include "regex.h"
#include "records.h"
#include "threads.h"

#define HELPSTR "Usage:\n"				\
    PACKAGE_NAME " [-d] [-t] [-c <file>] [-p <file>]\n"	\
    "-c <file>    read the config file <file>\n"	\
    "-d           run in foreground\n"			\
    "-h           this help\n"				\
    "-p <file>    write the pid to <file>\n"		\
    "-t           test configuration and exit\n"	\
    "-L           print license and exit\n"		\
    "-V           print version and exit\n"

#define LICENSESTR  "Copyright (C) 2010 Rafael Ostertag\n"		\
    "\n"								\
    PACKAGE_NAME " is free software: you can redistribute it and/or modify it under\n" \
    "the terms of the GNU General Public License as published by the Free\n" \
    "Software Foundation, either version 3 of the License, or (at your option)\n"	\
 "any later version." \
    "\n"								\
    "Foobar is distributed in the hope that it will be useful, but WITHOUT ANY\n" \
    "WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS\n"	\
    "FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more\n" \
    "details.\n"								\
    "\n"								\
    "You should have received a copy of the GNU General Public License along with\n" \
    "Foobar.  If not, see <http://www.gnu.org/licenses/>.\n"

#define VERSIONSTR PACKAGE_STRING " Copyright (C) 2010 Rafael Ostertag\n" 

static void
print_help() {
    fprintf(stdout, "%s", HELPSTR);
}

static void
print_license() {
    fprintf(stdout, "%s", LICENSESTR);
}

static void
print_version() {
    fprintf(stdout, "%s", VERSIONSTR);
}

int
main(int argc, char** argv) {
    FILE *pfile;
    pid_t pid;
    int c, testonly=0, daemonmode = 1, pidfile_from_cmdline = 0;
    char *cfgfile = NULL, optstr[] = "c:p:dthLV", *cmdline_pidfile = NULL;
    config *cfg;
    extern char *optarg;
    extern int optind, optopt, opterr;

    while ( (c = getopt(argc, argv, optstr) ) != -1) {
    	switch (c) {
    	case 'c':
    	    cfgfile = strdup(optarg);
    	    break;
	case 'p':
	    /* pid file specified on cmd line takes precedence */
	    cmdline_pidfile = strdup(optarg);
	    pidfile_from_cmdline = 1;
	    break;
    	case 'd':
	    daemonmode = 0;
    	    break;
    	case 'h':
    	    print_help();
    	    exit (0);
    	    break;
	case 't':
	    testonly = 1;
	    daemonmode = 0;
	    break;
    	case 'L':
    	    print_license();
    	    exit (0);
    	    break;
    	case 'V':
    	    print_version();
    	    exit(0);
    	    break;
    	case '?':
    	    fprintf(stderr, "Unknown argument %c\n", optopt);
    	    print_help();
    	    exit(1);
    	    break;
    	}
    }

    if ( daemonmode ) {
	out_settype(SYSLOG);
	if ( (pid = fork() ) < 0)
	    return(-1);
	else
	    if (pid != 0 )
		exit(0);/* Parent goes bye-bye */

	setsid();
	chdir("/");
	umask(0);
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
    } else {
	out_settype(CONSOLE);
    }

    if ( cfgfile == NULL )
	cfgfile = strdup(DEFAULT_CONFIGFILE);

    /* Read the configuration file */
    cfg = config_read(cfgfile);

    if ( pidfile_from_cmdline ) {
	assert ( cmdline_pidfile != NULL );
	strncpy(cfg->pidfile, cmdline_pidfile, _MAX_PATH);
    }

    if ( daemonmode ) {
	pfile = fopen(cfg->pidfile, "w");
	if ( pfile == NULL ) {
	    out_syserr(errno, "Error creating pid file '%s'", cfg->pidfile);
	    out_err("Exiting now.");
	    exit(1);
	}
	fprintf(pfile, "%li\n", getpid());
	fclose(pfile);
    }

    /* Compile the regexp */
    if ( testonly ) {
	out_msg("Testing configuration and exit...");
	regex_prepare();
	out_msg("Configuration appears to be fine.");
	exit (0);
    }
    regex_prepare();

    /* Initialize record vector */
    records_init();

    /* Set up the signal handlers */
    signalhandler_setup();

    /* Start threads */
    threads_start();

    out_msg("%s successfully started", argv[0]);

    /* This will only return upon exit or error */
    follow(cfg->syslogfile);

    threads_stop();

    records_destroy();

    if ( daemonmode ) {
	int retval;
	retval = unlink(cfg->pidfile);
	if ( retval != 0 ) {
	    out_syserr(errno, "Unable to unlink pid file '%s'", cfg->pidfile);
	}
    }

    out_msg("Exiting now.");
    
    /* Cleanup output to syslog */
    out_done();

    exit (0);
}
