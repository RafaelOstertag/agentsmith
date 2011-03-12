# $Id$

AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/src/network
if DEBUG
AM_CPPFLAGS+=-DDEBUG
else
AM_CPPFLAGS+=-DNDEBUG
endif

noinst_LIBRARIES = libclient.a

libclient_a_SOURCES = clientqueue.h clientqueue.c client.h client.c

CFLOW_FLAGS = --level-indent 2 --format posix --omit-arguments

libclient_a_CFLOW_INPUT=$(libclient_a_OBJECTS:.$(OBJEXT)=.c)
libclient_a.cflow: $(libclient_a_CFLOW_INPUT) Makefile
	cflow -o$@ $(CFLOW_FLAGS) $(DEFS) \
		$(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
		$(CPPFLAGS) \
		$(libclient_a_CFLOW_INPUT)
