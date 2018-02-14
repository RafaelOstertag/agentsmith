# $Id$

SUBDIRS = srv cli

AM_CPPFLAGS = -I$(top_srcdir)/src
if DEBUG
AM_CPPFLAGS+=-DDEBUG
else
AM_CPPFLAGS+=-DNDEBUG
endif

noinst_LIBRARIES = libnetshared.a

libnetshared_a_CFLAGS = $(STDC_CFLAGS)

libnetshared_a_SOURCES = netshared.h netshared.c netssl.h netssl.c
