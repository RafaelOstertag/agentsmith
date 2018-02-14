# $Id$
AC_PREREQ([2.65])
AC_INIT([agentsmith],[0.2],[rafi@guengel.ch])
AC_REVISION([$Rev$])
AC_CONFIG_SRCDIR([configure.ac])
AM_INIT_AUTOMAKE([silent-rules dist-bzip2 check-news])

# Checks for programs.
AC_PROG_INSTALL
AC_PROG_MKDIR_P
AC_PROG_CC
AC_PROG_SED
AC_PROG_RANLIB

AC_LANG([C])


AC_SUBST([CPPFLAGS],["$CPPFLAGS -D_POSIX_PTHREAD_SEMANTICS=1"])

# Checks for libraries.
AC_USE_SYSTEM_EXTENSIONS
AX_CHECK_COMPILE_FLAG([-std=c11], [AC_SUBST([STDC_CFLAGS],[-std=c11])])
AX_CHECK_SUNPRO_C(
	[AC_SUBST([CFLAGS],["$CFLAGS -mt"])],
	[:])
AC_CHECK_LIB([m],[ceil],,[
	AC_MSG_ERROR([libm was not found.])
])
AX_PTHREAD([
	AC_SUBST([LIBS],["$PTHREAD_LIBS $LIBS"])
	AC_SUBST([CFLAGS],["$CFLAGS $PTHREAD_CFLAGS"])
        AC_SUBST([LDFLAGS],["$LDFLAGS $PTHREAD_CFLAGS"])
	AC_SUBST([CC],["$PTHREAD_CC"])
	],
	[AC_MSG_ERROR([Threading not support on this platform])])

# Checks for header files.
AC_HEADER_TIME
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h assert.h errno.h inttypes.h math.h netdb.h netinet/in.h openssl/err.h openssl/ssl.h pthread.h semaphore.h signal.h stdio.h sys/select sys/socket.h sys/stat.h sys/types.h syslog.h unistd.h openssl/bio.h])
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
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_LONG_LONG_INT
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_SIZEOF([long])

AC_CHECK_TYPES([pthread_mutex_t, in_addr_t, struct addrinfo],,
	[AC_MSG_ERROR([Crucial data type not found])],
	[AC_INCLUDES_DEFAULT
	 #ifdef HAVE_PTHREAD_H
	 #include <pthread.h>
	 #endif
	 #ifdef HAVE_NETINET_IN_H
	 #include <netinet/in.h>
	 #endif
	 #ifdef HAVE_NETDB_H
	 #include <netdb.h>
	 #endif]
)

AC_CHECK_TYPES([struct in6_addr, in6_addr_t],,,
	[AC_INCLUDES_DEFAULT
	#ifdef HAVE_NETINET_IN_H
	#include <netinet/in.h>
	#endif]
)

# Solaris
AC_CHECK_MEMBER([struct in6_addr._S6_un._S6_u8],
	[AC_DEFINE([HAVE_SOLARIS_IN6_ADDR],
	[1],
	[Define to 1 if you compile on Sun Solaris])],,
	[AC_INCLUDES_DEFAULT
	#ifdef HAVE_NETINET_IN_H
	#include <netinet/in.h>
	#endif]
)
# *BSD
AC_CHECK_MEMBER([struct in6_addr.__u6_addr.__u6_addr8],
	[AC_DEFINE([HAVE_BSD_IN6_ADDR],
	[1],
	[Define to 1 if you compile on BSD])],,
	[AC_INCLUDES_DEFAULT
	#ifdef HAVE_NETINET_IN_H
	#include <netinet/in.h>
	#endif]
)
# LX
AC_CHECK_MEMBER([struct in6_addr.__in6_u.__u6_addr8],
	[AC_DEFINE([HAVE_LINUX_IN6_ADDR],
	[1],
	[Define to 1 if you compile on Linux])],,
	[AC_INCLUDES_DEFAULT
	#ifdef HAVE_NETINET_IN_H
	#include <netinet/in.h>
	#endif]
)

# Checks for library functions.
AC_SEARCH_LIBS([gethostbyname],[nsl])
AC_SEARCH_LIBS([connect],[socket])
AC_SEARCH_LIBS([sem_init],[rt])
AC_SEARCH_LIBS([nanosleep],[rt])
AC_SEARCH_LIBS([EVP_DecodeFinal],[crypto],,
	[AC_MSG_ERROR([libcrypto not found.])])
AC_SEARCH_LIBS([TLSv1_method],[ssl],,
	[AC_MSG_ERROR([libssl not found])],
	[-lcrypto])
AC_SEARCH_LIBS([pcre_compile],[pcre],,[
	AC_MSG_ERROR([libpcre not found.])
])
AC_SEARCH_LIBS([inet_pton],[c nsl],,[
        AC_MSG_ERROR([inet_pton() not found.])
])
AC_SEARCH_LIBS([inet_ntop],[c nsl],,[
        AC_MSG_ERROR([inet_ntop() not found.])
])

AC_FUNC_STRERROR_R
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_FORK

# Misc functions
AC_CHECK_FUNCS([accept atoi bind ceil connect ctime ctime_r freeaddrinfo gai_strerror getaddrinfo htonl inet_ntop inet_pton memset ntohl select sigaction sigprocmask socket strchr strdup strerror strncpy strsep usleep vsnprintf nanosleep])

AC_C_BIGENDIAN
# Enable
AC_SUBST([ENABLE_DEBUG],[no],[Whether or not debug code should be built])
AC_ARG_ENABLE(debug,
	[AS_HELP_STRING([--enable-debug],[Enable debug code])],
	[AC_SUBST([ENABLE_DEBUG],[$enableval])],
	[AC_SUBST([ENABLE_DEBUG],["no"])]
)

# ARGS
AC_SUBST([DEFAULT_CONFIGFILE],[$sysconfdir/agentsmith/agentsmith.conf],[The default location of the pid file])
AC_SUBST([DEFAULT_PIDFILE],[$localstatedir/agentsmith/agentsmith.pid],[The default location of the pid file])

AC_ARG_WITH([config],
	[AS_HELP_STRING([--with-config],
		[location of the configuration file @<:@default=SYSCONFDIR/agentsmith/agentsmith.conf@:>@])],
	[if test "x$withval" = xyes -o "x$withval" = xno ; then
	AC_MSG_FAILURE([--with-config expects a path])
	fi
	 AC_SUBST([DEFAULT_CONFIGFILE],[$withval])
	 ]
)
AC_ARG_WITH([pid],
	[AS_HELP_STRING([--with-pid],
		[location of the pid file @<:@default=LOCALSTATEDIR/agentsmith/agentsmith.pid@:>@])],
	[if test "x$withval" = xyes -o "x$withval" = xno; then
	AC_MSG_FAILURE([--with-pid expects a path])
	fi
	 AC_SUBST([DEFAULT_PIDFILE],[$withval])
	 ]
)

# Get kernel name
myOS=`uname -s 2>/dev/null`
case x$myOS in
     xSunOS)
     AC_DEFINE([KERNEL_SUNOS],[1],[Set to 1 if compiling on SunOS])
     
     ;;
     xFreeBSD)
     AC_DEFINE([KERNEL_FREEBSD],[1],[Set to 1 if compiling on FreeBSD])
     ;;
     xOpenBSD)
     AC_DEFINE([KERNEL_OPENBSD],[1],[Set to 1 if compiling on OpenBSD])
     ;;
     xNetBSD)
     AC_DEFINE([KERNEL_NETBSD],[1],[Set to 1 if compiling on NetBSD])
     ;;
     xLinux)
     AC_DEFINE([KERNEL_LINUX],[1],[Set to 1 if compiling on Linux])
     ;;
     *)
     AC_DEFINE([KERNEL_UNKNOWN],[1],[Set to 1 if compiling on Unknown Kernel])
     ;;
esac

# AM Conditionals
AM_CONDITIONAL([SUNOS],[test x$myOS = xSunOS])
AM_CONDITIONAL([DEBUG], [test x$ENABLE_DEBUG = xyes])

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	src/network/Makefile
	src/network/srv/Makefile
	src/network/cli/Makefile
	example/Makefile
	tests/Makefile
	doc/Makefile
])
AC_CONFIG_HEADERS([config.h])
AC_OUTPUT

echo ""
echo "Prefix:             $prefix"
echo "PID File:           $DEFAULT_PIDFILE"
echo "Configuration File: $DEFAULT_CONFIGFILE"
echo "With debug code:    $ENABLE_DEBUG"
echo ""
