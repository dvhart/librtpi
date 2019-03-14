# SPDX-License-Identifier: LGPL-2.1-only
# Copyright © 2018 VMware, Inc. All Rights Reserved.

AM_CPPFLAGS = -I. -I$(top_srcdir)/src

bin_PROGRAMS = test_api tst-condpi2 tst-cond1

test_api_SOURCES = test_api.c rtpi.h
test_api_LDADD = $(top_builddir)/src/librtpi.la -lpthread

tst_condpi2_SOURCES = tst-condpi2.c rtpi.h
tst_condpi2_LDADD = $(top_builddir)/src/librtpi.la -lpthread

tst_cond1_SOURCES = tst-cond1.c rtpi.h
tst_cond1_LDADD = $(top_builddir)/src/librtpi.la -lpthread

check_PROGRAMS = test_api tst-cond1 tst-condpi2
TESTS = test_api tst-cond1 tst-condpi2.sh
