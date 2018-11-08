# SPDX-License-Identifier: LGPL-2.1-only
# Copyright © 2018 VMware, Inc. All Rights Reserved.

noinst_LIBRARIES = librtpi.a
librtpi_a_SOURCES = rtpi.h rtpi_internal.h pi_futex.h pi_mutex.c pi_cond.c

bin_PROGRAMS = test tst-condpi2
test_SOURCES = test.c rtpi.h
test_LDADD = librtpi.a -lpthread

tst_condpi2_SOURCES = tst-condpi2.c rtpi.h
tst_condpi2_LDADD = librtpi.a -lpthread
