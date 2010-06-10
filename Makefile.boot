# $Id$

FILES = aclocal.m4 config.h.in configure depcomp install-sh Makefile.in missing	\
src/Makefile.in
DIRS = autom4te.cache

all: configure Makefile.in

configure: configure.ac config.h.in Makefile.boot
	aclocal
	autoconf

config.h.in: configure.ac Makefile.boot
	autoheader

Makefile.in: Makefile.boot Makefile.am src/Makefile.am
	automake --add-missing --copy --gnu

clean:
	rm -f $(FILES)
	rm -rf $(DIRS)
