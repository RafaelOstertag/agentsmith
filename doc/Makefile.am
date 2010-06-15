# $Id$

EXTRA_DIST = sshsentinel.1.in

man1_MANS = sshsentinel.1
CLEANFILES = $(man1_MANS)

do_subst = $(SED) -e 's,[@]PACKAGE[@],$(PACKAGE),g' \
	-e 's,[@]DEFAULT_PIDFILE[@],$(DEFAULT_PIDFILE),g' \
	-e 's,[@]DEFAULT_CONFIGFILE[@],$(DEFAULT_CONFIGFILE),g' \
	-e 's,[@]sysconfdir[@],$(sysconfdir),g' \
	-e 's,[@]mandir[@],$(mandir),g' \
	-e 's,[@]sbindir[@],$(sbindir),g'

sshsentinel.1: sshsentinel.1.in Makefile
	$(do_subst) < $(srcdir)/sshsentinel.1.in > $@
