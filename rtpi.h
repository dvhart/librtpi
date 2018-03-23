/* SPDX-License-Identifier: LGPL-2.1-only */
/* Copyright © 2018 VMware, Inc. All Rights Reserved. */

#ifndef _RTPI_H
#define _RTPI_H

#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <time.h>

/* TODO: Make these opaque types */
typedef struct pi_mutex {
	pthread_mutex_t mutex;
} pi_mutex_t;

typedef struct pi_cond {
	pthread_cond_t cond;
	pi_mutex_t *mutex;
} pi_cond_t;

/*
 * PI Mutex Interface
 */

#define RTPI_MUTEX_PSHARED    0x1
//#define RTPI_MUTEX_ROBUST     0x2
//#define RTPI_MUTEX_ERRORCHECK 0x4

int pi_mutex_init(pi_mutex_t *mutex, uint32_t flags);

int pi_mutex_destroy(pi_mutex_t *mutex);

int pi_mutex_lock(pi_mutex_t *mutex);

int pi_mutex_trylock(pi_mutex_t *mutex);

int pi_mutex_unlock(pi_mutex_t *mutex);


/*
 * PI Cond Interface
 */

#define RTPI_COND_PSHARED     0x1

int pi_cond_init(pi_cond_t *cond, pi_mutex_t *mutex, uint32_t flags);

int pi_cond_destroy(pi_cond_t *cond);

int pi_cond_wait(pi_cond_t *cond);

int pi_cond_timedwait(pi_cond_t *cond, const struct timespec *restrict abstime);

int pi_cond_signal(pi_cond_t *cond);

int pi_cond_broadcast(pi_cond_t *cond);

#endif // _RTPI_H
