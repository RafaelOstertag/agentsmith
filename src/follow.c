/* $Id$ */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
#error "stdlib.h is needed"
#endif
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else
#error "unistd.h is needed"
#endif

#include "follow.h"

static char BUFF[BUFFSIZE];
static char OUTBUF[BUFFSIZE];

void
buffout() {
    int retval;

    retval = snprintf(OUTBUF, BUFFSIZE,"%s",BUFF);
    if (retval == -1) {
	perror("Error");
	exit(0);
    }
    retval = printf("%s",OUTBUF);
    if (retval == -1) {
	perror("Error");
	exit(0);
    }
}

void
readtoeof (FILE *file) {
    static int buffpos = 0;
    int c;

    for (;;) {
	c = fgetc(file);
	if ( feof (file) ) {
	    clearerr(file);
	    break;
	}
	if ( ferror (file) ) {
	    perror("While reading");
	    exit (1);
	}
	
	BUFF[buffpos] = (unsigned char)c;
	buffpos++;
	if (buffpos == BUFFSIZE-1) {
	    BUFF[buffpos] = '\0';
	    buffpos=0;
	    buffout();
	    break;
	}
	if ( c == '\n' ) {
	    BUFF[buffpos] = '\0';
	    buffpos=0;
	    buffout();
	}
    }
}

void
follow (const char* fname) {
    struct stat sb;
    FILE *file;
    off_t curpos, lastpos;
    int retval;

    file = fopen(fname, "r");
    if (file == NULL) {
	perror("Error");
	exit (1);
    }

    retval = stat ( fname, &sb);
    if (retval == -1) {
	perror("Error");
	exit (1);
    }
    lastpos=curpos=sb.st_size;
    retval = fseek(file, curpos, SEEK_SET);
    if (retval == -1) {
	perror("Error");
	exit (1);
    }

    for (;;) {
	retval = stat(fname, &sb);
	if (retval == -1) {
	    perror("Error");
	    exit (1);
	}
	curpos=sb.st_size;
	if ( curpos > lastpos ) {
	    readtoeof(file);
	    lastpos=curpos;
	} else if ( lastpos > curpos ) {
	    rewind(file);
	    readtoeof(file);
	    lastpos=curpos;
	}
	sleep(1);
    }
    
}
