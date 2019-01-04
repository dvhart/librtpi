# SPDX-License-Identifier: LGPL-2.1-only
# Copyright © 2018 VMware, Inc. All Rights Reserved.

noinst_LIBRARIES = librtpi.a
librtpi_a_SOURCES = rtpi.h rtpi_internal.h pi_futex.h pi_mutex.c pi_cond.c

bin_PROGRAMS = test_api tst-condpi2 tst-cond1
test_api_SOURCES = test_api.c rtpi.h
test_api_LDADD = librtpi.a -lpthread

tst_condpi2_SOURCES = tst-condpi2.c rtpi.h
tst_condpi2_LDADD = librtpi.a -lpthread

tst_cond1_SOURCES = tst-cond1.c rtpi.h
tst_cond1_LDADD = librtpi.a -lpthread
