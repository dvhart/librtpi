// SPDX-License-Identifier: LGPL-2.1-only

#include <stdio.h>
#include "rtpi.h"

/*
 * This wrapper for early library validation only.
 * TODO: Replace with pthread_cond_t wrapper with a new cond implementation.
 *       Base this on the older version of the condvar, with the patch from
 *       Dinakar and Darren to enable priority fifo wakeup order.
 */
int pi_cond_init(pi_cond_t *cond, struct pi_mutex *mutex, uint32_t flags)
{
	pthread_condattr_t attr;
	struct timespec ts = { 0, 0 };
	int ret;

	ret = pthread_condattr_init(&attr);
	if (ret)
		goto out;

	/* All RTPI condvars are CLOCK_MONOTONIC */
	ret = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
	if (ret)
		goto out;

	if (flags && RTPI_COND_PSHARED) {
		ret = pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
		if (ret)
			goto out;
	}

	ret = pthread_cond_init(&cond->cond, &attr);
	if (ret)
		goto out;

	/* Force association with the specified mutex */
	ts.tv_nsec = 1;
	ret = pi_mutex_lock(mutex);
	if (ret)
		goto out;
	ret = pthread_cond_timedwait(&cond->cond, &mutex->mutex, &ts);
	if (ret == ETIMEDOUT)
		ret = 0;
	else if (ret == 0)
		ret = -EINVAL;

 out:
	return ret;
}

int pi_cond_destroy(pi_cond_t *cond)
{
	int ret;
	ret = pthread_cond_destroy(&cond->cond);
	return ret;
}

int pi_cond_wait(pi_cond_t *cond, pi_mutex_t *mutex)
{
	int ret;
	ret = pthread_cond_wait(&cond->cond, &mutex->mutex);
	return ret;
}

int pi_cond_timedwait(pi_cond_t *cond, pi_mutex_t *mutex, const struct timespec *restrict abstime)
{
	int ret;
	ret = pthread_cond_timedwait(&cond->cond, &mutex->mutex, abstime);
	return ret;
}

int pi_cond_signal(pi_cond_t *cond)
{
	int ret;
	ret = pthread_cond_signal(&cond->cond);
	return ret;
}

int pi_cond_broadcast(pi_cond_t *cond)
{
	int ret;
	ret = pthread_cond_broadcast(&cond->cond);
	return ret;
}

