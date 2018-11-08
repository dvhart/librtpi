// SPDX-License-Identifier: LGPL-2.1-only
// Copyright © 2018 VMware, Inc. All Rights Reserved.

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "rtpi_internal.h"
#include "pi_futex.h"

/*
 * This wrapper for early library validation only.
 * TODO: Replace with pthread_cond_t wrapper with a new cond implementation.
 *       Base this on the older version of the condvar, with the patch from
 *       Dinakar and Darren to enable priority fifo wakeup order.
 */

pi_cond_t *pi_cond_alloc(void)
{
	return malloc(sizeof(pi_cond_t));
}

void pi_cond_free(pi_cond_t *cond)
{
	free(cond);
}

int pi_cond_init(pi_cond_t *cond, struct pi_mutex *mutex, uint32_t flags)
{
	struct timespec ts = { 0, 0 };
	int ret;

	if (flags & ~(RTPI_COND_PSHARED)) {
		ret = -EINVAL;
		goto out;
	}

	if (flags & RTPI_COND_PSHARED) {
		cond->flags = RTPI_COND_PSHARED;
	}

	/* PSHARED has to match on both. */
	if ((cond->flags & RTPI_COND_PSHARED) ^ (mutex->flags % RTPI_MUTEX_PSHARED)) {
		ret = -EINVAL;
		goto out;
	}

	cond->mutex = mutex;

	ret = 0;
out:
	return ret;
}

int pi_cond_destroy(pi_cond_t *cond)
{
	memset(cond, 0, sizeof(*cond));
	return 0;
}

int pi_cond_wait(pi_cond_t *cond)
{
	int ret;

	ret = pi_mutex_unlock(cond->mutex);
	if (ret)
		return ret;
	ret = futex_wait_requeue_pi(cond, 0, NULL, cond->mutex);
	return ret;
}

int pi_cond_timedwait(pi_cond_t *cond, const struct timespec *restrict abstime)
{
	int ret;

	ret = pi_mutex_unlock(cond->mutex);
	if (ret)
		return ret;
	ret = futex_wait_requeue_pi(cond, 0, abstime, cond->mutex);
	return ret;
}

int pi_cond_signal(pi_cond_t *cond)
{
	int ret;

	ret = futex_cmp_requeue_pi(cond, 0, 0, cond->mutex);

	if (ret < 0)
		return ret;
	return 0;
}

int pi_cond_broadcast(pi_cond_t *cond)
{
	int ret;

	ret = futex_cmp_requeue_pi(cond, 0, INT_MAX, cond->mutex);

	if (ret < 0)
		return ret;
	return 0;
}
