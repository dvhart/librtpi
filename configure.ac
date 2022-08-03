# SPDX-License-Identifier: LGPL-2.1-only

AC_INIT([librtpi], [0.0.1], [dvhart@infradead.org])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AM_PROG_AR
LT_INIT
AC_PROG_CC
AC_PROG_CXX
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
 Makefile
 src/Makefile
 tests/Makefile
 tests/glibc-tests/Makefile
 tests/libstdc++-tests/Makefile
])
AC_CONFIG_MACRO_DIRS([m4])
AX_CXX_COMPILE_STDCXX_11
AC_OUTPUT
