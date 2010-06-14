# $Id$

sbin_PROGRAMS = sshsentinel
AM_CPPFLAGS = -DDEFAULT_PIDFILE='"$(DEFAULT_PIDFILE)"' -DDEFAULT_CONFIGFILE='"$(DEFAULT_CONFIGFILE)"'
if DEBUG
AM_CPPFLAGS+=-DDEBUG
else
AM_CPPFLAGS+=-DNDEBUG
endif

sshsentinel_SOURCES = globals.h main.c cfg.h cfg.c output.h output.c		\
sighandler.h sighandler.c regex.h regex.c follow.h follow.c records.h records.c	\
threads.h threads.c
