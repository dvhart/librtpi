/* Copyright (C) 2003-2019 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2003.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "rtpi.h"

#if defined _POSIX_CLOCK_SELECTION && _POSIX_CLOCK_SELECTION >= 0
static int run_test(clockid_t cl)
{
	pi_cond_t cond;
	pi_mutex_t mut;
	uint32_t flags = 0;

	printf("clock = %d\n", (int)cl);

	if (cl == CLOCK_REALTIME) {
#ifdef RTPI_COND_CLOCK_REALTIME
		flags = RTPI_COND_CLOCK_REALTIME;
#else
		puts("CLOCK_REALTIME not supported");
		return 1;
#endif
	}

	if (pi_mutex_init(&mut, 0) != 0) {
		puts("mutex_init failed");
		return 1;
	}

	if (pi_cond_init(&cond, &mut, flags) != 0) {
		puts("cond_init failed");
		return 1;
	}

	if (pi_mutex_lock(&mut) != 0) {
		puts("mutex_lock failed");
		return 1;
	}

	if (pi_mutex_lock(&mut) != EDEADLK) {
		puts("2nd mutex_lock did not return EDEADLK");
		return 1;
	}

	struct timespec ts;
	if (clock_gettime(cl, &ts) != 0) {
		puts("clock_gettime failed");
		return 1;
	}

	/* Wait one second.  */
	++ts.tv_sec;

	int e = pi_cond_timedwait(&cond, &ts);
	if (e == 0) {
		puts("cond_timedwait succeeded");
		return 1;
	} else if (e != ETIMEDOUT) {
		puts("cond_timedwait did not return ETIMEDOUT");
		return 1;
	}

	struct timespec ts2;
	if (clock_gettime(cl, &ts2) != 0) {
		puts("second clock_gettime failed");
		return 1;
	}

	if (ts2.tv_sec < ts.tv_sec
	    || (ts2.tv_sec == ts.tv_sec && ts2.tv_nsec < ts.tv_nsec)) {
		puts("timeout too short");
		return 1;
	}

	if (pi_mutex_unlock(&mut) != 0) {
		puts("mutex_unlock failed");
		return 1;
	}

	if (pi_mutex_destroy(&mut) != 0) {
		puts("mutex_destroy failed");
		return 1;
	}

	if (pi_cond_destroy(&cond) != 0) {
		puts("cond_destroy failed");
		return 1;
	}

	return 0;
}
#endif

static int do_test(void)
{
#if !defined _POSIX_CLOCK_SELECTION || _POSIX_CLOCK_SELECTION == -1

	puts("_POSIX_CLOCK_SELECTION not supported, test skipped");
	return 0;

#else

	int res = run_test(CLOCK_REALTIME);

#if defined _POSIX_MONOTONIC_CLOCK && _POSIX_MONOTONIC_CLOCK >= 0
#if _POSIX_MONOTONIC_CLOCK == 0
	int e = sysconf(_SC_MONOTONIC_CLOCK);
	if (e < 0)
		puts("CLOCK_MONOTONIC not supported");
	else if (e == 0) {
		puts("sysconf (_SC_MONOTONIC_CLOCK) must not return 0");
		res = 1;
	} else
#endif
		res |= run_test(CLOCK_MONOTONIC);
#else
	puts("_POSIX_MONOTONIC_CLOCK not defined");
#endif

	return res;
#endif
}

#include "test-driver.c"
