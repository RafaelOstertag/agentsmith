# $Id$
AC_PREREQ([2.65])
AC_INIT([sshsentinel],[0.1],[rafi@guengel.ch])
AC_REVISION([$Rev$])
AC_CONFIG_SRCDIR([configure.ac])
AM_INIT_AUTOMAKE([dist-bzip2 silent-rules])
AM_SILENT_RULES

# Checks for programs.
AC_PROG_INSTALL
AC_PROG_MKDIR_P
AC_PROG_CC
AC_PROG_SED

# Checks for libraries.
AC_USE_SYSTEM_EXTENSIONS
AX_CHECK_SUNPRO_C([AC_SUBST([CFLAGS],["$CFLAGS -mt"])],[:])
AC_CHECK_LIB([m],[ceil],,[
	AC_MSG_ERROR([libm was not found.])
])
AX_PTHREAD([
	AC_SUBST([LIBS],["$PTHREAD_LIBS $LIBS"])
	AC_SUBST([CFLAGS],["$CFLAGS $PTHREAD_CFLAGS"])
	AC_SUBST([CC],["$PTHREAD_CC"])
	],
	[AC_MSG_ERROR([Threading not support on this platform])])

# Checks for header files.
AC_HEADER_TIME
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([stdio.h syslog.h unistd.h errno.h assert.h signal.h sys/types.h sys/stat.h pthread.h math.h])
AC_CHECK_HEADER([pcre.h],
	AC_DEFINE([HAVE_PCRE_H],
	[1],
	[Define to 1 if you have the <pcre.h> header file]),
	[AC_CHECK_HEADER([pcre/pcre.h],
	AC_DEFINE([HAVE_PCRE_PCRE_H],
	[1],
	[Define to 1 if you have the <pcre/pcre.h> header file]),[
	AC_MSG_ERROR([pcre.h was not found.])
	])
])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_CHECK_TYPES([pthread_mutex_t],,
	[AC_MSG_ERROR([Crucial data type was not found])],
	[AC_INCLUDES_DEFAULT
         #ifdef HAVE_PTHREAD_H
         #include <pthread.h>
         #endif]
)

# Checks for library functions.
AC_FUNC_STRERROR_R
AC_CHECK_LIB([pcre],[pcre_compile],,[
	AC_MSG_ERROR([libpcre was not found.])
])
AC_CHECK_FUNCS([pcre_study pcre_exec pcre_fullinfo],,[
        AC_MSG_ERROR([libpcre lacks crucial function(s)])])
AC_CHECK_FUNCS([strerror vsnprintf strsep strchr usleep strdup memset sigprocmask sigaction pthread_mutex_init pthread_mutex_destroy realloc ceil ctime_r ctime])

# Enable
AC_SUBST([ENABLE_DEBUG],[no],[Whether or not debug code should be built])
AC_ARG_ENABLE(debug,
	[AS_HELP_STRING([--enable-debug],[Enable debug code])],
	[AC_SUBST([ENABLE_DEBUG],[$enableval])],
	[AC_SUBST([ENABLE_DEBUG],["no"])]
)

# ARGS
AC_SUBST([DEFAULT_CONFIGFILE],[$sysconfdir/sshsentinel.conf],[The default location of the pid file])
AC_SUBST([DEFAULT_PIDFILE],[$localstatedir/sshsentinel/sshsentinel.pid],[The default location of the pid file])

AC_ARG_WITH([config],
	[AS_HELP_STRING([--with-config],
		[location of the configuration file @<:@default=SYSCONFDIR/sshsentinel.conf@:>@])],
	[if test "x$withval" = xyes -o "x$withval" = xno ; then
	AC_MSG_FAILURE([--with-config expects a path])
        fi
         AC_SUBST([DEFAULT_CONFIGFILE],[$withval])
	 ]
)
AC_ARG_WITH([pid],
	[AS_HELP_STRING([--with-pid],
		[location of the pid file @<:@default=LOCALSTATEDIR/sshsentinel/sshsentinel.pid@:>@])],
	[if test "x$withval" = xyes -o "x$withval" = xno; then
	AC_MSG_FAILURE([--with-pid expects a path])
        fi
	 AC_SUBST([DEFAULT_PIDFILE],[$withval])
	 ]
)

# AM Conditionals
AM_CONDITIONAL([DEBUG], [test x$ENABLE_DEBUG = xyes])

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	example/Makefile
	tests/Makefile
	doc/Makefile
])
AC_CONFIG_HEADERS([config.h])
AC_OUTPUT

echo ""
echo "PID File:           $DEFAULT_PIDFILE"
echo "Configuration File: $DEFAULT_CONFIGFILE"
echo "Debug Code:         $ENABLE_DEBUG"
echo ""
