/* Copyright (C) 2002-2019 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2002.

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
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "rtpi.h"

static DEFINE_PI_MUTEX(mut, 0);
static DEFINE_PI_COND(cond, 0);

static int do_test(void)
{
	int err;
	struct timespec ts;

	/* Get the mutex.  */
	if (pi_mutex_lock(&mut) != 0) {
		puts("mutex_lock failed");
		exit(1);
	}

	/* Waiting for the condition will fail.  But we want the timeout here.  */
	err = clock_gettime(CLOCK_MONOTONIC, &ts);
	if (err != 0) {
		printf("clock_gettime failed with error %s\n", strerror(err));
		exit(1);
	}

	ts.tv_nsec += 500000000;
	if (ts.tv_nsec >= 1000000000) {
		ts.tv_nsec -= 1000000000;
		++ts.tv_sec;
	}
	err = pi_cond_timedwait(&cond, &mut, &ts);
	if (err == 0) {
		/* This could in theory happen but here without any signal and
		   additional waiter it should not.  */
		puts("cond_timedwait succeeded");
		exit(1);
	} else if (err != ETIMEDOUT) {
		printf("cond_timedwait returned with %s\n", strerror(err));
		exit(1);
	}

	err = pi_mutex_unlock(&mut);
	if (err != 0) {
		printf("mutex_unlock failed: %s\n", strerror(err));
		exit(1);
	}

	return 0;
}

#include "test-driver.c"
