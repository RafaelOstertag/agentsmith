# $Id$

EXTRA_DIST = agentsmith.man.in agentsmith.conf.man.in

if SUNOS
man1_MANS = agentsmith.man
man4_MANS = agentsmith.conf.man agentsmith_exclude.man
CLEANFILES = $(man1_MANS) $(man4_MANS)
ADM_SECTION = 1
FILE_SECTION = 4
else
man8_MANS = agentsmith.man
man5_MANS = agentsmith.conf.man agentsmith_exclude.man
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

agentsmith.man: agentsmith.man.in Makefile
	$(do_subst) < $(srcdir)/agentsmith.man.in > $@

agentsmith.conf.man: agentsmith.conf.man.in Makefile
	$(do_subst) < $(srcdir)/agentsmith.conf.man.in > $@

agentsmith_exclude.man: agentsmith_exclude.man.in Makefile
	$(do_subst) < $(srcdir)/agentsmith_exclude.man.in > $@
