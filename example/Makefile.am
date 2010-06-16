# $Id$

etc_DATA = sshsentinel.conf.example
etcdir = $(sysconfdir)/sshsentinel

etcsshsentinel_DATA = action.sh.mail action.sh.iptables action.sh.pf
etcsshsentineldir = $(sysconfdir)/sshsentinel/action

example_DATA = sshsentinel.xml sshsentinel
exampledir = $(pkgdatadir)/example

CLEANFILES = $(example_DATA) $(etc_DATA) $(etcsshsentinel_DATA)
EXTRA_DIST = sshsentinel.conf.in sshsentinel.xml.in sshsentinel.in	\
action.sh.mail.in action.sh.pf.in action.sh.iptables.in

do_subst = $(SED) -e 's,[@]PACKAGE[@],$(PACKAGE),g' \
	-e 's,[@]DEFAULT_PIDFILE[@],$(DEFAULT_PIDFILE),g' \
	-e 's,[@]DEFAULT_CONFIGFILE[@],$(DEFAULT_CONFIGFILE),g' \
	-e 's,[@]sysconfdir[@],$(sysconfdir),g' \
	-e 's,[@]mandir[@],$(mandir),g' \
	-e 's,[@]sbindir[@],$(sbindir),g'

sshsentinel.conf.example: sshsentinel.conf.in Makefile
	$(do_subst) < $(srcdir)/sshsentinel.conf.in > $@

sshsentinel.xml: sshsentinel.xml.in Makefile
	$(do_subst) < $(srcdir)/sshsentinel.xml.in > $@

sshsentinel: sshsentinel.in Makefile
	$(do_subst) < $(srcdir)/sshsentinel.in > $@

action.sh.mail: action.sh.mail.in Makefile
	$(do_subst) < $(srcdir)/action.sh.mail.in > $@

action.sh.iptables: action.sh.iptables.in Makefile
	$(do_subst) < $(srcdir)/action.sh.iptables.in > $@

action.sh.pf: action.sh.pf.in Makefile
	$(do_subst) < $(srcdir)/action.sh.pf.in > $@
