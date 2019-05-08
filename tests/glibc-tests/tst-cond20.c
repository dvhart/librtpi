/* Copyright (C) 2004-2019 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Jakub Jelinek <jakub@redhat.com>, 2004.

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

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "rtpi.h"

#define N 10
#define ROUNDS 1000
static DEFINE_PI_MUTEX(mut, 0);
static DEFINE_PI_COND(cond, &mut, 0);
static DEFINE_PI_COND(cond2, &mut, 0);
static pthread_barrier_t b;
static int count;

static void *tf(void *p)
{
	int i;
	int err;

	for (i = 0; i < ROUNDS; ++i) {
		pi_mutex_lock(&mut);

		if (++count == N)
			pi_cond_signal(&cond2);

#ifdef TIMED
		struct timespec ts;
		/* Wait three seconds.  */
		err = clock_gettime(CLOCK_MONOTONIC, &ts);
		if (err != 0) {
			puts("child: clock_gettime failed");
			exit(1);
		}
		ts.tv_sec += 3;
		pi_cond_timedwait(&cond, &ts);
#else
		pi_cond_wait(&cond);
#endif

		pi_mutex_unlock(&mut);

		err = pthread_barrier_wait(&b);
		if (err != 0 && err != PTHREAD_BARRIER_SERIAL_THREAD) {
			puts("child: barrier_wait failed");
			exit(1);
		}

		err = pthread_barrier_wait(&b);
		if (err != 0 && err != PTHREAD_BARRIER_SERIAL_THREAD) {
			puts("child: barrier_wait failed");
			exit(1);
		}
	}

	return NULL;
}

static int do_test(void)
{
	if (pthread_barrier_init(&b, NULL, N + 1) != 0) {
		puts("barrier_init failed");
		return 1;
	}

	pi_mutex_lock(&mut);

	int i, j, err;
	pthread_t th[N];
	for (i = 0; i < N; ++i)
		if ((err = pthread_create(&th[i], NULL, tf, NULL)) != 0) {
			printf("cannot create thread %d: %s\n", i,
			       strerror(err));
			return 1;
		}

	for (i = 0; i < ROUNDS; ++i) {
		/* Make sure we discard spurious wake-ups.  */
		do
			pi_cond_wait(&cond2);
		while (count != N);

		if (i & 1)
			pi_mutex_unlock(&mut);

		if (i & 2)
			pi_cond_broadcast(&cond);
		else if (i & 4)
			for (j = 0; j < N; ++j)
				pi_cond_signal(&cond);
		else {
			for (j = 0; j < (i / 8) % N; ++j)
				pi_cond_signal(&cond);
			pi_cond_broadcast(&cond);
		}

		if ((i & 1) == 0)
			pi_mutex_unlock(&mut);

		err = pi_cond_destroy(&cond);
		if (err) {
			printf("pi_cond_destroy failed: %s\n",
			       strerror(err));
			return 1;
		}

		/* Now clobber the cond variable which has been successfully
		   destroyed above.  */
		memset(&cond, (char)i, sizeof(cond));

		err = pthread_barrier_wait(&b);
		if (err != 0 && err != PTHREAD_BARRIER_SERIAL_THREAD) {
			puts("parent: barrier_wait failed");
			return 1;
		}

		pi_mutex_lock(&mut);

		err = pthread_barrier_wait(&b);
		if (err != 0 && err != PTHREAD_BARRIER_SERIAL_THREAD) {
			puts("parent: barrier_wait failed");
			return 1;
		}

		count = 0;
		err = pi_cond_init(&cond, &mut, 0);
		if (err) {
			printf("pi_cond_init failed: %s\n", strerror(err));
			return 1;
		}
	}

	for (i = 0; i < N; ++i)
		if ((err = pthread_join(th[i], NULL)) != 0) {
			printf("failed to join thread %d: %s\n", i,
			       strerror(err));
			return 1;
		}

	puts("done");

	return 0;
}

#include "test-driver.c"
