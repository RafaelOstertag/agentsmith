# $Id$

EXTRA_DIST = sshsentinel.man.in

if SUNOS
man1_MANS = sshsentinel.man
CLEANFILES = $(man1_MANS)
ADM_SECTION = 1
else
man8_MANS = sshsentinel.man
CLEANFILES = $(man8_MANS)
ADM_SECTION = 8
endif

do_subst = $(SED) -e 's,[@]PACKAGE[@],$(PACKAGE),g' \
	-e 's,[@]DEFAULT_PIDFILE[@],$(DEFAULT_PIDFILE),g' \
	-e 's,[@]DEFAULT_CONFIGFILE[@],$(DEFAULT_CONFIGFILE),g' \
	-e 's,[@]sysconfdir[@],$(sysconfdir),g' \
	-e 's,[@]mandir[@],$(mandir),g' \
	-e 's,[@]ADM_SECTION[@],$(ADM_SECTION),g' \
	-e 's,[@]sbindir[@],$(sbindir),g'

sshsentinel.man: sshsentinel.man.in Makefile
	$(do_subst) < $(srcdir)/sshsentinel.man.in > $@
