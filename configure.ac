#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([dice], [0.9], [https://notabug.org/cryptarch/dice.git])
AM_INIT_AUTOMAKE([-Wall -Werror])
AC_CONFIG_SRCDIR([io.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([m], [ceil])
AC_CHECK_LIB([ncurses], [tgetent])
AC_CHECK_LIB([readline], [readline])

# Checks for header files.
AC_CHECK_HEADERS([limits.h stddef.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([clock_gettime memset strerror strtol])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
