# $Id$

AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/src/network
if DEBUG
AM_CPPFLAGS+=-DDEBUG
else
AM_CPPFLAGS+=-DNDEBUG
endif

noinst_LIBRARIES = libclient.a

libclient_a_SOURCES = clientqueue.h clientqueue.c client.h client.c
