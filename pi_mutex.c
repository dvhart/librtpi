// SPDX-License-Identifier: LGPL-2.1-only
// Copyright © 2018 VMware, Inc. All Rights Reserved.

#include "rtpi_internal.h"
#include "pi_futex.h"
#include <stdbool.h>
#include <string.h>

static pid_t gettid(void)
{
	static __thread pid_t tid_this_thread;

	if (tid_this_thread)
		return tid_this_thread;

	tid_this_thread = syscall(SYS_gettid);
	return tid_this_thread;
}

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
	if (pi_mutex_trylock(mutex))
		return 0;
	return futex_lock_pi(mutex);
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
	pid_t pid;
	bool ret;

	pid = gettid();
	ret = __sync_bool_compare_and_swap(&mutex->futex,
					   pid, 0);
	if (ret == true)
		return 0;
	return futex_unlock_pi(mutex);
}
