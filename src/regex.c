
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
#include "cfg.h"
#include "regex.h"
#include "output.h"
#include "records.h"
#include "exclude.h"

static int regexp_compiled = 0;
static pcre *compiled_regexp = NULL;
static pcre_extra *compiled_regexp_extra = NULL;
static char *pcre_errptr = NULL;
static int pcre_erroffset = 0;

void
regex_prepare() {
    int       retval, capturecount;

    compiled_regexp =
	pcre_compile(CONFIG.regex, 0, (const char **) &pcre_errptr,
		     &pcre_erroffset, NULL);
    if (compiled_regexp == NULL) {
	out_err("Unable to compile '%s': %s (Position: %i)",
		CONFIG.regex, pcre_errptr, pcre_erroffset);
	exit(2);
    }
    compiled_regexp_extra =
	pcre_study(compiled_regexp, 0, (const char **) &pcre_errptr);
    if (pcre_errptr != NULL) {
	out_err("Unable to study '%s': %s", CONFIG.regex, pcre_errptr);
	exit(2);
    }

    retval = pcre_fullinfo(compiled_regexp,
			   compiled_regexp_extra,
			   PCRE_INFO_CAPTURECOUNT, &capturecount);
    if (retval != 0) {
	out_err
	    ("Error obtaining information on compiled regular expression. The return code was: %i",
	     retval);
	exit(2);
    }

    if (capturecount != 1) {
	out_err
	    ("You must exactly use 1 ('one') capture pattern in the regular expression");
	exit(2);
    }

    regexp_compiled = 1;
}

void
regex_do(const char *buff) {
    char      match[BUFFSIZE];
    int       retval;
    int       ovector[30];

    if (!regexp_compiled)
	regex_prepare();

    retval = pcre_exec(compiled_regexp,
		       compiled_regexp_extra,
		       buff, strlen(buff), 0, 0, ovector, 30);
    if (retval == 2) {
	int       match_len;
	match_len = ovector[3] - ovector[2];
	memset(match, 0, BUFFSIZE);
	strncpy(match, buff + ovector[2], match_len);
	out_dbg("Found match '%s'", match);

	/*
	 * Check if this match is excluded.
	 */
	if (exclude_isexcluded(match) != 0) {
	    retval = records_add_ip(match);
	    if (retval != 0) {
		out_err("Got error adding '%s' to record vector (retval==%i)",
			match, retval);
	    }
	} else {
	    out_msg("%s has been ignored due to entry in exclude file",
		    match);
	}
    }
}
