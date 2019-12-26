# SPDX-License-Identifier: LGPL-2.1-only
# Copyright © 2019 National Instruments. All Rights Reserved.

AM_CPPFLAGS = -I. -I$(top_srcdir)/src
LDADD = $(top_builddir)/src/librtpi.la -lpthread

check_PROGRAMS = tst-cond1 tst-cond2 tst-cond3 tst-cond9

TESTS = tst-cond1 tst-cond2 tst-cond3 tst-cond9

test: check
