// SPDX-License-Identifier: LGPL-2.1-only
// Copyright © 2018 VMware, Inc. All Rights Reserved.

#include "rtpi_internal.h"

pi_mutex_t *pi_mutex_alloc(void)
{
	return malloc(sizeof(pi_mutex_t));
}

void pi_mutex_free(pi_mutex_t *mutex)
{
	free(mutex);
}

int pi_mutex_init(pi_mutex_t *mutex, uint32_t flags)
{
	pthread_mutexattr_t attr;
	int ret;

	ret = pthread_mutexattr_init(&attr);
	if (ret)
		goto out;

	/* All RTPI mutexes are PRIO_INHERIT */
	ret = pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);
	if (ret)
		goto out;

	if (flags && RTPI_MUTEX_PSHARED) {
		ret = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
		if (ret)
			goto out;
	}

	ret = pthread_mutex_init(&mutex->mutex, &attr);

 out:
	return ret;
}

int pi_mutex_destroy(pi_mutex_t *mutex)
{
	int ret;
	/* TODO: should we also destroy the mutexattr? */
	ret = pthread_mutex_destroy(&mutex->mutex);
	return ret;
}

int pi_mutex_lock(pi_mutex_t *mutex)
{
	int ret;
	ret = pthread_mutex_lock(&mutex->mutex);
	return ret;
}

int pi_mutex_trylock(pi_mutex_t *mutex)
{
	int ret;
	ret = pthread_mutex_trylock(&mutex->mutex);
	return ret;
}

int pi_mutex_unlock(pi_mutex_t *mutex)
{
	int ret;
	ret = pthread_mutex_unlock(&mutex->mutex);
	return ret;
}

