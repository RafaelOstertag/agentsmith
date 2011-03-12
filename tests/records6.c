
/* $Id$
 *
 * Tests the records functions multi threaded.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include "globals.h"
#include "records.h"
#include "output.h"

#define BUFFSIZE 256
#define ITERATIONS 32
#define SLEEP_TIME 30

unsigned long num_entries = 0;

int
callback(hostrecord_t *ptr) {
    num_entries++;
    return RETVAL_OK;
}

void     *
thread_add(void *wdc) {
    char      str[BUFFSIZE];
    unsigned long i, k;
    int       retval;

    for (;;) {
	for (i = 0; i < ITERATIONS; i++) {
	    for (k = 0; k < ITERATIONS; k++) {
		snprintf(str, BUFFSIZE, "192.168.%i.%i", i, k);
		retval = records_add_ip(str);
		if (retval != 0) {
		    fprintf(stderr, "records_add_ip() returned %i\n", retval);
		    exit(1);
		}
	    }
	}
	pthread_testcancel();
    }
}

void     *
thread_remove(void *wdc) {
    char      str[BUFFSIZE];
    unsigned long i, k;
    int       retval;

    for (;;) {
	for (i = 0; i < ITERATIONS; i++) {
	    for (k = 0; k < ITERATIONS; k++) {
		snprintf(str, BUFFSIZE, "192.168.%i.%i", i, k);
		retval = records_remove(str);
	    }
	}
	pthread_testcancel();
    }
}

void     *
thread_enumerate(void *wdc) {
    int       retval;

    for (;;) {
	retval = records_enumerate(callback, SYNC);
	if (retval != 0) {
	    out_err("records_enumerate() returned value %i", retval);
	    exit(1);
	}
	pthread_testcancel();
    }
}

int
main(int wdc1, char **wdc2) {
    int       retval;
    hostrecord_t *ptr;
    char      str[BUFFSIZE];
    unsigned long i, k;
    pthread_t pth_add, pth_remove, pth_enumerate;
#ifdef HAVE_NANOSLEEP
    struct timespec time_wait, time_wait_remaining;
#endif

    out_msg("This test will take at least %i seconds.\n", SLEEP_TIME);

    records_init();

    retval = pthread_create(&pth_add, NULL, thread_add, NULL);
    if (retval != 0) {
	out_syserr(errno, "Error creating thread thread_add()");
	exit(1);
    }
    retval = pthread_create(&pth_remove, NULL, thread_remove, NULL);
    if (retval != 0) {
	out_syserr(errno, "Error creating thread thread_add()");
	exit(1);
    }
    retval = pthread_create(&pth_enumerate, NULL, thread_enumerate, NULL);
    if (retval != 0) {
	out_syserr(errno, "Error creating thread thread_add()");
	exit(1);
    }

    out_msg("Waiting %i seconds", SLEEP_TIME);
#ifdef HAVE_NANOSLEEP
	time_wait.tv_sec = SLEEP_TIME;
	time_wait.tv_nsec = 0;
	nanosleep(&time_wait, &time_wait_remaining);
#else

#warning "Using sleep()"
	sleep(SLEEP_TIME);
#endif

    out_msg("Cancelling thread thread_add()");
    retval = pthread_cancel(pth_add);
    if (retval != 0) {
	out_syserr(errno, "Error cancelling thread thread_add()");
	exit(1);
    }
    retval = pthread_join(pth_add, NULL);
    if (retval != 0) {
	out_syserr(errno, "Error cancelling thread thread_add()");
	exit(1);
    }
    out_msg("Joined thread thread_add()");

    out_msg("Cancelling thread thread_remove()");
    retval = pthread_cancel(pth_remove);
    if (retval != 0) {
	out_syserr(errno, "Error cancelling thread thread_remove()");
	exit(1);
    }
    retval = pthread_join(pth_remove, NULL);
    if (retval != 0) {
	out_syserr(errno, "Error cancelling thread thread_remove()");
	exit(1);
    }
    out_msg("Joined thread thread_remove()");

    out_msg("Cancelling thread thread_enumerate()");
    retval = pthread_cancel(pth_enumerate);
    if (retval != 0) {
	out_syserr(errno, "Error cancelling thread thread_enumerate()");
	exit(1);
    }
    retval = pthread_join(pth_enumerate, NULL);
    if (retval != 0) {
	out_syserr(errno, "Error cancelling thread thread_enumerate()");
	exit(1);
    }
    out_msg("Joined thread thread_enumerate()");

    records_destroy(NULL);
    exit(0);
}
