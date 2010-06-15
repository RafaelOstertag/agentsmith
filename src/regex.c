/* Copyright (C) 2010 Rafael Ostertag
 *
 * This file is part of sshsentinel.
 *
 * sshsentinel is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * sshsentinel is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * sshsentinel.  If not, see <http://www.gnu.org/licenses/>.
 */

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_PCRE_H
#include <pcre.h>
#elif HAVE_PCRE_PCRE_H
#include <pcre/pcre.h>
#else
#error "pcre.h not found"
#endif

#ifdef STDC_HEADERS
#include <string.h>
#endif

#include "globals.h"
#include "regex.h"
#include "output.h"
#include "records.h"
#include "cfg.h"

static int regexp_compiled = 0;
static pcre *compiled_regexp = NULL;
static pcre_extra *compiled_regexp_extra = NULL;
static char *pcre_errptr = NULL;
static int pcre_erroffset = 0;

void
regex_prepare() {
    int retval, capturecount;
    config *cfg;

    cfg = config_get();
    if ( cfg == NULL ) {
	out_err("<NULL> config received in regex_prepare()");
	exit (2);
    }

    compiled_regexp = pcre_compile(cfg->regex, 0, (const char**)&pcre_errptr, &pcre_erroffset, NULL);
    if ( compiled_regexp == NULL ) {
	out_err("Unable to compile '%s': %s (Position: %i)",
		cfg->regex,
		pcre_errptr,
		pcre_erroffset);
	exit (2);
    }
    compiled_regexp_extra = pcre_study(compiled_regexp, 0, (const char**)&pcre_errptr);
    if ( pcre_errptr != NULL ) {
	out_err("Unable to study '%s': %s",
		cfg->regex,
		pcre_errptr);
	exit (2);
    }

    retval = pcre_fullinfo ( compiled_regexp,
			     compiled_regexp_extra,
			     PCRE_INFO_CAPTURECOUNT,
			     &capturecount );
    if ( retval != 0 ) {
	out_err("Error obtaining information on compiled regular expression. The return code was: %i", retval);
	exit (2);
    }

    if ( capturecount != 1 ) {
	out_err("You must exactly use 1 ('one') capture pattern in the regular expression");
	exit (2);
    }

    regexp_compiled = 1;
}

void
regex_do(const char* buff) {
    char match[BUFFSIZE];
    int retval;
    int ovector[30];

    if ( !regexp_compiled )
	regex_prepare();

    retval = pcre_exec(compiled_regexp,
		       compiled_regexp_extra,
		       buff,
		       strlen(buff),
		       0,
		       0,
		       ovector,
		       30);
    if ( retval == 2 ) {
	int match_len;
	match_len = ovector[3] - ovector[2];
	memset(match, 0, BUFFSIZE);
	strncpy(match, buff + ovector[2], match_len);
	out_dbg("Found match '%s'", match);
	retval = records_add(match);
	if (retval != 0) {
	    out_err("Got error adding '%s' to record vector (retval==%i)",
		    match,
		    retval);
	}
    }
}
