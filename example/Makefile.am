# $Id$

etc_DATA = sshsentinel.conf.example
etcdir = $(sysconfdir)

etcsshsentinel_DATA = action.sh.example
etcsshsentineldir = $(sysconfdir)/sshsentinel.d

example_DATA = sshsentinel.xml sshsentinel
exampledir = $(pkgdatadir)/example

CLEANFILES = $(example_DATA) $(etc_DATA) $(etcsshsentinel_DATA)
EXTRA_DIST = sshsentinel.conf.in sshsentinel.xml.in sshsentinel.in action.sh.in

do_subst = $(SED) -e 's,[@]PACKAGE[@],$(PACKAGE),g' \
	-e 's,[@]DEFAULT_PIDFILE[@],$(DEFAULT_PIDFILE),g' \
	-e 's,[@]DEFAULT_CONFIGFILE[@],$(DEFAULT_CONFIGFILE),g' \
	-e 's,[@]sysconfdir[@],$(sysconfdir),g' \
	-e 's,[@]mandir[@],$(mandir),g' \
	-e 's,[@]sbindir[@],$(sbindir),g'

sshsentinel.conf.example: sshsentinel.conf.in Makefile
	$(do_subst) < $< > $@

sshsentinel.xml: sshsentinel.xml.in Makefile
	$(do_subst) < $< > $@

sshsentinel: sshsentinel.in Makefile
	$(do_subst) < $< > $@

action.sh.example: action.sh.in Makefile
	$(do_subst) < $< > $@
