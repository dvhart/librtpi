# SPDX-License-Identifier: LGPL-2.1-only

noinst_LIBRARIES = librtpi.a
librtpi_a_SOURCES = rtpi.h pi_mutex.c pi_cond.c

bin_PROGRAMS = test
test_SOURCES = test.c rtpi.h
test_LDADD = librtpi.a -lpthread
