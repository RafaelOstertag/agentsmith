# $Id$

SUBDIRS = network

sbin_PROGRAMS = agentsmith
AM_CPPFLAGS = -I$(top_srcdir)/src/network -I$(top_srcdir)/src/network/srv \
-I$(top_srcdir)/src/network/cli -DDEFAULT_PIDFILE='"$(DEFAULT_PIDFILE)"'  \
-DDEFAULT_CONFIGFILE='"$(DEFAULT_CONFIGFILE)"'

if DEBUG
AM_CPPFLAGS+=-DDEBUG
else
AM_CPPFLAGS+=-DNDEBUG
endif

agentsmith_CFLAGS = $(STDC_CFLAGS)

agentsmith_LDADD = network/srv/libserver.a network/cli/libclient.a	\
network/libnetshared.a

agentsmith_SOURCES = globals.h main.c cfg.h cfg.c output.h output.c		\
sighandler.h sighandler.c regex.h regex.c follow.h follow.c records.h records.c	\
threads.h threads.c exclude.c exclude.h malloc.c
