AC_PREREQ(2.61)
AC_INIT([mcalloc], [0.2], [mike@conlen.org])
AC_CONFIG_SRCDIR([src/test.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CANONICAL_HOST
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([-Wall -Werror])
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CPP
AM_PROG_AR
LT_INIT
AC_CONFIG_MACRO_DIR([m4])

AC_HEADER_STDC

CPPFLAGS="$CPPFLAGS"
CFLAGS="$CFLAGS -std=c99 -Wall -Werror -Wpedantic -Wno-deprecated-declarations -Wno-unused-function -Wno-unused-variable"
CXXFLAGS="$CXXFLAGS --std=c++0x -Wall -Werror -Wpedantic -Wno-deprecated-declarations -Wno-unused-function -Wno-unused-variable"
LDFLAGS="$LDFLAGS"

case $host_os in
	darwin* )
		CFLAGS="$CFLAGS -I/opt/local/include"
		CXXFLAGS="$CXXFLAGS -I/opt/local/include"
		CPPFLAGS="$CPPFLAGS -I/opt/local/include"
		LDFLAGS="$LDFLAGS -L/opt/local/lib"
	;;
	linux*)
		# configure won't find getpagesize without this	
		CPPFLAGS="$CPPFLAGS -D_BSD_SOURCE=1"
	;;
	*BSD*)
		# Do something specific for BSD
	;;
	*)
		#Default Case
		AC_MSG_ERROR([Your platform is not currently supported])
	;;
esac


AC_SEARCH_LIBS([CU_initialize_registry], [cunit], [
	AC_CHECK_HEADERS([CUnit/CUnit.h], [], [])
], [ ])


AC_CHECK_HEADERS([malloc.h])
AC_CHECK_HEADERS([stdio.h])
AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([sys/malloc.h])
AC_CHECK_HEADERS([malloc/malloc.h])

AC_FUNC_MMAP
AC_CHECK_FUNC(sbrk)

AC_CHECK_FUNC(malloc_stats)

AC_CHECK_FUNC(getenv)
AC_CHECK_FUNC(secure_getenv)

AC_CHECK_TYPES([uint64_t], [
	AC_CHECK_SIZEOF([uint64_t])
], [
	AC_CHECK_SIZEOF([uint32_t])
], [#include <stdint.h>])
AC_TYPE_SIZE_T
AC_CHECK_SIZEOF([void *])
AC_CHECK_SIZEOF([size_t])

AC_CHECK_TYPES([struct mallinfo], [], [], [
#ifdef HAVE_MALLOC_H
	#include <malloc.h>
#endif
#ifdef HAVE_MALLOC_MALLOC_H
	#include <malloc/malloc.h>
#endif
#ifdef HAVE_SYS_MALLOC_H
	#include <sys/malloc.h>
#endif
])

AC_CONFIG_FILES([Makefile
	src/Makefile])
AC_OUTPUT
