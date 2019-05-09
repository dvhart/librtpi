# SPDX-License-Identifier: LGPL-2.1-only
# Copyright © 2018 VMware, Inc. All Rights Reserved.

AM_CPPFLAGS = -I. -I$(top_srcdir)/src
LDADD = $(top_builddir)/src/librtpi.la -lpthread
SUBDIRS = glibc-tests

check_PROGRAMS = test_api tst-cond1 tst-condpi2
TESTS = test_api tst-cond1 tst-condpi2.sh
