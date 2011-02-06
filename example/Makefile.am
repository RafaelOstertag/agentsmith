# $Id$

etc_DATA = agentsmith.conf.example exclude.example
etcdir = $(sysconfdir)/agentsmith

etcagentsmith_DATA = action.sh.mail action.sh.iptables action.sh.pf
etcagentsmithdir = $(sysconfdir)/agentsmith/action

example_DATA = agentsmith.xml agentsmith
exampledir = $(pkgdatadir)/example

CLEANFILES = $(example_DATA) $(etc_DATA) $(etcagentsmith_DATA)
EXTRA_DIST = agentsmith.conf.in agentsmith.xml.in agentsmith.in			\
action.sh.mail.in action.sh.pf.in action.sh.iptables.in exclude.example.in

do_subst = $(SED) -e 's,[@]PACKAGE[@],$(PACKAGE),g' \
	-e 's,[@]DEFAULT_PIDFILE[@],$(DEFAULT_PIDFILE),g' \
	-e 's,[@]DEFAULT_CONFIGFILE[@],$(DEFAULT_CONFIGFILE),g' \
	-e 's,[@]sysconfdir[@],$(sysconfdir),g' \
	-e 's,[@]mandir[@],$(mandir),g' \
	-e 's,[@]sbindir[@],$(sbindir),g'

agentsmith.conf.example: agentsmith.conf.in Makefile
	$(do_subst) < $(srcdir)/agentsmith.conf.in > $@

exclude.example: exclude.example.in Makefile
	$(do_subst) < $(srcdir)/exclude.example.in > $@

agentsmith.xml: agentsmith.xml.in Makefile
	$(do_subst) < $(srcdir)/agentsmith.xml.in > $@

agentsmith: agentsmith.in Makefile
	$(do_subst) < $(srcdir)/agentsmith.in > $@

action.sh.mail: action.sh.mail.in Makefile
	$(do_subst) < $(srcdir)/action.sh.mail.in > $@

action.sh.iptables: action.sh.iptables.in Makefile
	$(do_subst) < $(srcdir)/action.sh.iptables.in > $@

action.sh.pf: action.sh.pf.in Makefile
	$(do_subst) < $(srcdir)/action.sh.pf.in > $@
