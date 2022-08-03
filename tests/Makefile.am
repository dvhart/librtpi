# SPDX-License-Identifier: LGPL-2.1-only
# Copyright © 2018 VMware, Inc. All Rights Reserved.

AM_CPPFLAGS = -I. -I$(top_srcdir)/src
LDADD = $(top_builddir)/src/librtpi.la -lpthread
SUBDIRS = glibc-tests libstdc++-tests

check_PROGRAMS = test_api tst-cond1 tst-condpi2 tst-condpi2-cpp
TESTS = test_api tst-cond1 tst-condpi2.sh tst-condpi2-cpp.sh

tst_condpi2_cpp_SOURCES = tst-condpi2-cpp.cpp
