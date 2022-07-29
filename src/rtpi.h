/* SPDX-License-Identifier: LGPL-2.1-only */
/* Copyright © 2018 VMware, Inc. All Rights Reserved. */

#ifndef RTPI_H
#define RTPI_H

#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

#include "rtpi_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef union pi_mutex pi_mutex_t;
typedef union pi_cond pi_cond_t;

/*
 * PI Mutex Interface
 */
#define DEFINE_PI_MUTEX(mutex, flags) \
	pi_mutex_t mutex = PI_MUTEX_INIT(flags)

#define RTPI_MUTEX_PSHARED    0x1
//#define RTPI_MUTEX_ROBUST     0x2
//#define RTPI_MUTEX_ERRORCHECK 0x4

pi_mutex_t *pi_mutex_alloc(void);

void pi_mutex_free(pi_mutex_t *mutex);

int pi_mutex_init(pi_mutex_t *mutex, uint32_t flags);

int pi_mutex_destroy(pi_mutex_t *mutex);

int pi_mutex_lock(pi_mutex_t *mutex);

int pi_mutex_trylock(pi_mutex_t *mutex);

int pi_mutex_unlock(pi_mutex_t *mutex);


/*
 * PI Cond Interface
 */
#define DEFINE_PI_COND(condvar, flags) \
	pi_cond_t condvar = PI_COND_INIT(flags)

#define RTPI_COND_PSHARED     RTPI_MUTEX_PSHARED

pi_cond_t *pi_cond_alloc(void);

void pi_cond_free(pi_cond_t *cond);

int pi_cond_init(pi_cond_t *cond, uint32_t flags);

int pi_cond_destroy(pi_cond_t *cond);

int pi_cond_wait(pi_cond_t *cond, pi_mutex_t *mutex);

int pi_cond_timedwait(pi_cond_t *cond, pi_mutex_t *mutex,
		      const struct timespec *abstime);

int pi_cond_signal(pi_cond_t *cond, pi_mutex_t *mutex);

int pi_cond_broadcast(pi_cond_t *cond, pi_mutex_t *mutex);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // RTPI_H
