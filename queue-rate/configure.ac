#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.69])
AC_INIT([FULL-PACKAGE-NAME], [VERSION], [BUG-REPORT-ADDRESS])
AC_CONFIG_MACRO_DIR([autoconf-archive/m4])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_CONFIG_SRCDIR([src])
AM_CONFIG_HEADER(config.h)

AC_CONFIG_FILES([Makefile])

: ${CFLAGS="-O3"}
: ${CXXFLAGS="-O3"}

# Checks for programs.
dnl AC_PROG_CC([icc gcc cc])
AC_PROG_CC
AC_PROG_CXX

AC_ARG_WITH([hwloc-include],
	[AC_HELP_STRING([--with-hwloc-include=PATH],
		[specify path where hwloc include directory can be found])],
	[AS_CASE(["$withval"],
		[yes|no|''],
		[AC_MSG_WARN([--with-hwloc-include=PATH expects a valid PATH])])],
	[])
AC_ARG_WITH([hwloc-lib],
	[AC_HELP_STRING([--with-hwloc-lib=PATH],
		[specify path where hwloc lib directory can be found])],
	[AS_CASE(["$withval"],
		[yes|no|''],
		[AC_MSG_WARN([--with-hwloc-lib=PATH expects a valid PATH])])],
	[])
AC_ARG_WITH([hwloc],
	[AC_HELP_STRING([--with-hwloc=PATH],
		[specify path where hwloc include and lib directory can be found])],
	[AS_CASE(["$withval"],
		[yes|no|''],
		[AC_MSG_WARN([--with-hwloc=PATH expects a valid PATH])],
		with_hwloc_include="${with_hwloc_include:-$withval/include}"; with_hwloc_lib="${with_hwloc_lib:-$withval/lib}")],
	[])

AS_IF([test -n "$with_hwloc_include"],
	[AC_CHECK_HEADER([${with_hwloc_include}/hwloc.h],
		[AC_SUBST(hwloc_inc, [-I$with_hwloc_include])],
		[AC_MSG_WARN([specified hwloc include directory ('$with_hwloc_include') is not valid])])])

AS_IF([test -n "$with_hwloc_lib"],
	[LDFLAGS="$LDFLAGS -L${with_hwloc_lib}"])

AC_CHECK_LIB([hwloc], [hwloc_topology_init])

# Checks for libraries.
AX_PTHREAD
AC_SUBST(pthread_libs, [$PTHREAD_LIBS])
AC_SUBST(pthread_cflags, [$PTHREAD_CFLAGS])


# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h unistd.h])

AC_CHECK_HEADER([stdatomic.h], [], [AC_MSG_ERROR([C11 with atomic support needed.])])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset strtoul strtoull])

AC_OUTPUT
