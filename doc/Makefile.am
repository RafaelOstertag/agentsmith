# $Id$

EXTRA_DIST = sshsentinel.man.in sshsentinel.conf.man.in

if SUNOS
man1_MANS = sshsentinel.man
man4_MANS = sshsentinel.conf.man
CLEANFILES = $(man1_MANS) $(man4_MANS)
ADM_SECTION = 1
FILE_SECTION = 4
else
man8_MANS = sshsentinel.man
man5_MANS = sshsentinel.conf.man
CLEANFILES = $(man8_MANS) $(man5_MANS)
ADM_SECTION = 8
FILE_SECTION = 5
endif

do_subst = $(SED) -e 's,[@]PACKAGE[@],$(PACKAGE),g' \
	-e 's,[@]DEFAULT_PIDFILE[@],$(DEFAULT_PIDFILE),g' \
	-e 's,[@]DEFAULT_CONFIGFILE[@],$(DEFAULT_CONFIGFILE),g' \
	-e 's,[@]sysconfdir[@],$(sysconfdir),g' \
	-e 's,[@]mandir[@],$(mandir),g' \
	-e 's,[@]ADM_SECTION[@],$(ADM_SECTION),g' \
	-e 's,[@]FILE_SECTION[@],$(FILE_SECTION),g' \
	-e 's,[@]sbindir[@],$(sbindir),g'

sshsentinel.man: sshsentinel.man.in Makefile
	$(do_subst) < $(srcdir)/sshsentinel.man.in > $@

sshsentinel.conf.man: sshsentinel.conf.man.in Makefile
	$(do_subst) < $(srcdir)/sshsentinel.conf.man.in > $@
