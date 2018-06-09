// SPDX-License-Identifier: LGPL-2.1-only
// Copyright © 2018 VMware, Inc. All Rights Reserved.

#include "rtpi_internal.h"
#include "pi_futex.h"
#include <stdbool.h>
#include <string.h>

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

	/* All RTPI mutexes are PRIO_INHERIT */
	memset(mutex, 0, sizeof(*mutex));

	/* Check for unknown options */
	if (flags & ~(RTPI_MUTEX_PSHARED)) {
		ret = -EINVAL;
		goto out;
	}

	if (flags & RTPI_MUTEX_PSHARED)
		mutex->flags = RTPI_MUTEX_PSHARED;
	ret = 0;
out:
	return ret;
}

int pi_mutex_destroy(pi_mutex_t *mutex)
{
	memset(mutex, 0, sizeof(*mutex));
	return 0;
}

int pi_mutex_lock(pi_mutex_t *mutex)
{
	int ret;

	ret = futex_lock_pi(&mutex->futex);
	return ret;
}

int pi_mutex_trylock(pi_mutex_t *mutex)
{
	pid_t pid;
	bool ret;

	pid = gettid();
	ret = __sync_bool_compare_and_swap(&mutex->futex,
					   0, pid);
	if (ret == true)
		return 1;
	return 0;
}

int pi_mutex_unlock(pi_mutex_t *mutex)
{
	int ret;

	ret = futex_unlock_pi(&mutex->futex);
	return ret;
}

