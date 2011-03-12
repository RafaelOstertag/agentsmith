# $Id$

AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/src/network
if DEBUG
AM_CPPFLAGS+=-DDEBUG
else
AM_CPPFLAGS+=-DNDEBUG
endif

noinst_LIBRARIES = libserver.a

libserver_a_SOURCES = server.h server.c worker.h worker.c

CFLOW_FLAGS = --level-indent 2 --format posix --omit-arguments

libserver_a_CFLOW_INPUT=$(libserver_a_OBJECTS:.$(OBJEXT)=.c)
libserver_a.cflow: $(libserver_a_CFLOW_INPUT) Makefile
	cflow -o$@ $(CFLOW_FLAGS) $(DEFS) \
		$(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
		$(CPPFLAGS) \
		$(libserver_a_CFLOW_INPUT)
