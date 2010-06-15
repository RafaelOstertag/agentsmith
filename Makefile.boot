# -*- makefile -*-
#
# $Id$

FILES = aclocal.m4 config.h.in configure depcomp install-sh Makefile.in missing	\
src/Makefile.in example/Makefile.in tests/Makefile.in config.log config.sub	\
config.guess core config.log.1
DIRS = autom4te.cache

all: configure Makefile.in

configure: configure.ac config.h.in Makefile.boot
	aclocal
	autoconf 

config.h.in: configure.ac Makefile.boot
	autoheader -f

Makefile.in: configure Makefile.boot Makefile.am src/Makefile.am example/Makefile.am tests/Makefile.am doc/Makefile.am
	automake --add-missing --copy --gnu

clean:
	rm -f $(FILES)
	rm -rf $(DIRS)
