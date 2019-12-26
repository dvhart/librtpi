// SPDX-License-Identifier: LGPL-2.1-only
// Copyright © 2018 VMware, Inc. All Rights Reserved.

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "rtpi.h"
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

int pi_cond_init(pi_cond_t *cond, pi_mutex_t *mutex, uint32_t flags)
{
	struct timespec ts = { 0, 0 };
	int ret;

	if (flags & ~(RTPI_COND_PSHARED)) {
		ret = EINVAL;
		goto out;
	}
	memset(cond, 0, sizeof(*cond));
	if (flags & RTPI_COND_PSHARED) {
		cond->flags = RTPI_COND_PSHARED;
	}

	/* PSHARED has to match on both. */
	if ((cond->flags & RTPI_COND_PSHARED) ^ (mutex->flags % RTPI_MUTEX_PSHARED)) {
		ret = EINVAL;
		goto out;
	}
	pi_mutex_init(&cond->priv_mut, cond->flags & RTPI_COND_PSHARED);
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
	__u32 wait_id;
	__u32 futex_id;

	ret = pi_mutex_lock(&cond->priv_mut);
	if (ret)
		return ret;

	ret = pi_mutex_unlock(cond->mutex);
	if (ret) {
		pi_mutex_unlock(&cond->priv_mut);
		return ret;
	}
	cond->pending_wait++;
	cond->cond++;
	wait_id = cond->cond;
	do {

		futex_id = cond->cond;
		pi_mutex_unlock(&cond->priv_mut);

		ret = futex_wait_requeue_pi(cond, futex_id, NULL, cond->mutex);
		if (ret < 0) {
			if (errno == EAGAIN) {
				/* futex VAL changed between unlock & wait */
				pi_mutex_lock(&cond->priv_mut);
				if (cond->wake_id >= wait_id && cond->pending_wake) {
					/* There is one wakeup pending for us */
					cond->pending_wake--;
					cond->pending_wait--;
					pi_mutex_unlock(&cond->priv_mut);
					pi_mutex_lock(cond->mutex);
					ret = 0;
					break;
				}
				/* Reload VAL and try again */
				continue;
			} else {
				/* Error, abort */
				pi_mutex_lock(&cond->priv_mut);
				cond->pending_wait--;
				pi_mutex_unlock(&cond->priv_mut);
				ret = errno;
				break;
			}
		}
		/* All good. Proper wakeup + we own the lock */
		if (cond->pending_wake) {
			cond->pending_wait--;
			cond->pending_wake--;
			pi_mutex_unlock(&cond->priv_mut);
			pi_mutex_lock(cond->mutex);
			ret = 0;
			break;
		}
		/* No wakeup for us, try again… */
	} while (1);
	return ret;
}

int pi_cond_timedwait(pi_cond_t *cond, const struct timespec *restrict abstime)
{
	int ret;

	ret = pi_mutex_unlock(cond->mutex);
	if (ret)
		return ret;
	ret = futex_wait_requeue_pi(cond, 0, abstime, cond->mutex);
	return (ret) ? errno : 0;
}

int pi_cond_signal(pi_cond_t *cond)
{
	int ret;
	__u32 id;

	pi_mutex_lock(&cond->priv_mut);

	if (!cond->pending_wait) {
		/* No waiters pending */
		pi_mutex_unlock(&cond->priv_mut);
		return 0;
	}
	cond->cond++;
	id = cond->cond;
	cond->wake_id = id;
	cond->pending_wake++;
	pi_mutex_unlock(&cond->priv_mut);

	do {
		ret = futex_cmp_requeue_pi(cond, id, 0, cond->mutex);
		if (ret > 0) {
			/* Wakeup performed */
			break;
		} else if (ret == 0) {
			/* nothing woke up */
			pi_mutex_lock(&cond->priv_mut);
			cond->pending_wake--;
			pi_mutex_unlock(&cond->priv_mut);
		} else if (errno == EAGAIN) {
			/* id changed */
			pi_mutex_lock(&cond->priv_mut);
			cond->cond++;
			id = cond->cond;
			cond->wake_id = id;
			pi_mutex_unlock(&cond->priv_mut);
		} else {
			return errno;
		}
	} while (1);
	return 0;
}

int pi_cond_broadcast(pi_cond_t *cond)
{
	int ret;
	__u32 id;

	pi_mutex_lock(&cond->priv_mut);

	if (!cond->pending_wait) {
		/* No waiters pending */
		pi_mutex_unlock(&cond->priv_mut);
		return 0;
	}
	cond->cond++;
	id = cond->cond;
	cond->wake_id = id;
	cond->pending_wake = cond->pending_wait;
	pi_mutex_unlock(&cond->priv_mut);

	do {
		ret = futex_cmp_requeue_pi(cond, id, INT_MAX, cond->mutex);
		if (ret >= 0) {
			/* Wakeup performed */
			break;
		} else if (errno == EAGAIN) {
			/* id changed */
			pi_mutex_lock(&cond->priv_mut);
			cond->cond++;
			id = cond->cond;
			cond->wake_id = id;
			cond->pending_wake = cond->pending_wait;
			pi_mutex_unlock(&cond->priv_mut);
		} else {
			return errno;
		}
	} while (1);
	return 0;
}
