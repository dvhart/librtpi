/* Verify that condition variables synchronized by PI mutexes don't hang on
   on cancellation.
   Copyright (C) 2012-2019 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

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
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <time.h>

#include "rtpi.h"

#define NUM 5
#define ITERS 10000
#define COUNT 100

typedef void *(*thr_func) (void *);

pi_mutex_t mutex;
pi_cond_t cond;

void cleanup(void *u)
{
	/* pi_cond_wait should always return with the mutex locked.  The
	   pi_mutex_unlock implementation does not actually check whether we
	   own the mutex for several mutex kinds, so check this explicitly.  */
	int ret = pi_mutex_trylock(&mutex);
	if (ret != EDEADLK && ret != EBUSY) {
		printf("mutex not locked in cleanup %d\n", ret);
		abort();
	}
	if (pi_mutex_unlock(&mutex))
		abort();
}

void *signaller(void *u)
{
	int i, ret = 0;
	void *tret = NULL;

	for (i = 0; i < ITERS; i++) {
		if ((ret = pi_mutex_lock(&mutex)) != 0) {
			tret = (void *)1;
			printf("signaller:mutex_lock failed: %s\n",
			       strerror(ret));
			goto out;
		}
		if ((ret = pi_cond_signal(&cond)) != 0) {
			tret = (void *)1;
			printf("signaller:signal failed: %s\n", strerror(ret));
			goto unlock_out;
		}
		if ((ret = pi_mutex_unlock(&mutex)) != 0) {
			tret = (void *)1;
			printf("signaller:mutex_unlock failed: %s\n",
			       strerror(ret));
			goto out;
		}
		pthread_testcancel();
	}

out:
	return tret;

unlock_out:
	if ((ret = pi_mutex_unlock(&mutex)) != 0)
		printf("signaller:mutex_unlock[2] failed: %s\n", strerror(ret));
	goto out;
}

void *waiter(void *u)
{
	int i, ret = 0;
	void *tret = NULL;
	int seq = (uintptr_t) u;

	for (i = 0; i < ITERS / NUM; i++) {
		if ((ret = pi_mutex_lock(&mutex)) != 0) {
			tret = (void *)(uintptr_t) 1;
			printf("waiter[%u]:mutex_lock failed: %s\n", seq,
			       strerror(ret));
			goto out;
		}
		pthread_cleanup_push(cleanup, NULL);

		if ((ret = pi_cond_wait(&cond)) != 0) {
			tret = (void *)(uintptr_t) 1;
			printf("waiter[%u]:wait failed: %s\n", seq,
			       strerror(ret));
			goto unlock_out;
		}

		if ((ret = pi_mutex_unlock(&mutex)) != 0) {
			tret = (void *)(uintptr_t) 1;
			printf("waiter[%u]:mutex_unlock failed: %s\n", seq,
			       strerror(ret));
			goto out;
		}
		pthread_cleanup_pop(0);
	}

out:
	puts("waiter tests done");
	return tret;

unlock_out:
	if ((ret = pi_mutex_unlock(&mutex)) != 0)
		printf("waiter:mutex_unlock[2] failed: %s\n", strerror(ret));
	goto out;
}

void *timed_waiter(void *u)
{
	int i, ret;
	void *tret = NULL;
	int seq = (uintptr_t) u;

	for (i = 0; i < ITERS / NUM; i++) {
		struct timespec ts;

		if ((ret = clock_gettime(CLOCK_MONOTONIC, &ts)) != 0) {
			tret = (void *)(uintptr_t) 1;
			printf("%u:clock_gettime failed: %s\n", seq,
			       strerror(errno));
			goto out;
		}
		ts.tv_sec += 20;

		if ((ret = pi_mutex_lock(&mutex)) != 0) {
			tret = (void *)(uintptr_t) 1;
			printf("waiter[%u]:mutex_lock failed: %s\n", seq,
			       strerror(ret));
			goto out;
		}
		pthread_cleanup_push(cleanup, NULL);

		/* We should not time out either.  */
		if ((ret = pi_cond_timedwait(&cond, &ts)) != 0) {
			tret = (void *)(uintptr_t) 1;
			printf("waiter[%u]:timedwait failed: %s\n", seq,
			       strerror(ret));
			goto unlock_out;
		}
		if ((ret = pi_mutex_unlock(&mutex)) != 0) {
			tret = (void *)(uintptr_t) 1;
			printf("waiter[%u]:mutex_unlock failed: %s\n", seq,
			       strerror(ret));
			goto out;
		}
		pthread_cleanup_pop(0);
	}

out:
	puts("timed_waiter tests done");
	return tret;

unlock_out:
	if ((ret = pi_mutex_unlock(&mutex)) != 0)
		printf("waiter[%u]:mutex_unlock[2] failed: %s\n", seq,
		       strerror(ret));
	goto out;
}

int do_test_wait(thr_func f)
{
	pthread_t w[NUM];
	pthread_t s;
	int i, j, ret = 0;
	void *thr_ret;

	for (i = 0; i < COUNT; i++) {
		if ((ret = pi_mutex_init(&mutex, 0)) != 0) {
			printf("mutex_init failed: %s\n", strerror(ret));
			goto out;
		}

		if ((ret = pi_cond_init(&cond, &mutex, 0)) != 0) {
			printf("cond_init failed: %s\n", strerror(ret));
			goto out;
		}

		for (j = 0; j < NUM; j++)
			if ((ret = pthread_create(&w[j], NULL,
						  f,
						  (void *)(uintptr_t) j)) !=
			    0) {
				printf("waiter[%d]: create failed: %s\n", j,
				       strerror(ret));
				goto out;
			}

		if ((ret = pthread_create(&s, NULL, signaller, NULL)) != 0) {
			printf("signaller: create failed: %s\n", strerror(ret));
			goto out;
		}

		for (j = 0; j < NUM; j++) {
			pthread_cancel(w[j]);

			if ((ret = pthread_join(w[j], &thr_ret)) != 0) {
				printf("waiter[%d]: join failed: %s\n", j,
				       strerror(ret));
				goto out;
			}

			if (thr_ret != NULL && thr_ret != PTHREAD_CANCELED) {
				ret = 1;
				goto out;
			}
		}

		/* The signalling thread could have ended before it was cancelled.  */
		pthread_cancel(s);

		if ((ret = pthread_join(s, &thr_ret)) != 0) {
			printf("signaller: join failed: %s\n", strerror(ret));
			goto out;
		}

		if (thr_ret != NULL && thr_ret != PTHREAD_CANCELED) {
			ret = 1;
			goto out;
		}
	}

out:
	return ret;
}

int do_test(int argc, char **argv)
{
	int ret = do_test_wait(waiter);

	if (ret)
		return ret;

	return do_test_wait(timed_waiter);
}

#define TEST_FUNCTION_ARGV do_test
#include "test-driver.c"
