# $Id$

SUBDIRS = srv cli

AM_CPPFLAGS = -I$(top_srcdir)/src
if DEBUG
AM_CPPFLAGS+=-DDEBUG
else
AM_CPPFLAGS+=-DNDEBUG
endif

noinst_LIBRARIES = libnetshared.a
libnetshared_a_SOURCES = netshared.h netshared.c netssl.h netssl.c

CFLOW_FLAGS = --level-indent 2 --format posix --omit-arguments

libnetshared_a_CFLOW_INPUT=$(libnetshared_a_OBJECTS:.$(OBJEXT)=.c)
libnetshared_a.cflow: $(libnetshared_a_CFLOW_INPUT) Makefile
	cflow -o$@ $(CFLOW_FLAGS) $(DEFS) \
		$(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
		$(CPPFLAGS) \
		$(libnetshared_a_CFLOW_INPUT)
