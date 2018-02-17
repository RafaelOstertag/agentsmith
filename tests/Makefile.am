# $Id$
AUTOMAKE_OPTIONS=color-tests

TESTS = records1 records2 records3 records4 records5 records6 records7 exclude1
check_PROGRAMS = records1 records2 records3 records4 records5 records6 records7 exclude1

needed_files = $(top_srcdir)/src/records.c $(top_srcdir)/src/output.c	\
$(top_srcdir)/src/cfg.c $(top_srcdir)/src/exclude.c
AM_CPPFLAGS = -I$(top_srcdir)/src -DDEFAULT_PIDFILE='"$(DEFAULT_PIDFILE)"' \
-DDEFAULT_CONFIGFILE='"$(DEFAULT_CONFIGFILE)"' -DCHECK
if DEBUG
AM_CPPFLAGS+=-DDEBUG
else
AM_CPPFLAGS+=-DNDEBUG
endif

AM_CFLAGS = $(STDC_CFLAGS)

records1_SOURCES = records1.c $(needed_files)
records2_SOURCES = records2.c $(needed_files)
records3_SOURCES = records3.c $(needed_files)
records4_SOURCES = records4.c $(needed_files)
records5_SOURCES = records5.c $(needed_files)
records6_SOURCES = records6.c $(needed_files)
records7_SOURCES = records7.c $(needed_files)
exclude1_SOURCES = exclude1.c $(needed_files)
