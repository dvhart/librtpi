# SPDX-License-Identifier: LGPL-2.1-only
#
# This is a massively simplified Makefile.am for tests from the libstdc++
# test suite that have been ported over to use librtpi primitives.
#
# Notably, we do not include the compile-only tests (e.g. the ones to make
# sure that compilation fails when trying to use a deleted member, et al)
# because trying to get the scripting (some of which is in dejagnu) over
# here for that was more effort than it seemed worth.

AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -I. -I$(top_srcdir)/src -I$(top_srcdir)/tests/libstdc++-tests/util
LDADD = $(top_builddir)/src/librtpi.la -lpthread

test_list = \
    condition_variable/cons/1 \
    condition_variable/members/1 \
    condition_variable/members/2 \
    condition_variable/members/53841 \
    condition_variable/native_handle/typesizes \
    condition_variable/requirements/standard_layout \
    condition_variable/requirements/typedefs \
    mutex/cons/1 \
    mutex/cons/constexpr \
    mutex/dest/destructor_locked \
    mutex/lock/1 \
    mutex/native_handle/1 \
    mutex/native_handle/typesizes \
    mutex/requirements/standard_layout \
    mutex/requirements/typedefs \
    mutex/try_lock/1 \
    mutex/try_lock/2 \
    mutex/unlock/1 \
    mutex/unlock/2

# need to support notify_all_at_thread_exit
#    condition_variable/members/3

# pi_cond_destroy needs to unblock all waiters
#    condition_variable/54185

# these ones haven't been evaluated yet
#    condition_variable/members/68519
#    condition_variable/members/103382

AM_DEFAULT_SOURCE_EXT = .cc

nobase_check_PROGRAMS = $(test_list)

TESTS = $(test_list)

test: check
