/* SPDX-License-Identifier: LGPL-2.1-only */
/* Copyright © 2018 VMware, Inc. All Rights Reserved. */

#ifndef _RTPI_H
#define _RTPI_H

#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

typedef struct pi_mutex pi_mutex_t;
typedef struct pi_cond pi_cond_t;

/*
 * PI Mutex Interface
 */

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

#define RTPI_COND_PSHARED     RTPI_MUTEX_PSHARED

pi_cond_t *pi_cond_alloc(void);

void pi_cond_free(pi_cond_t *cond);

int pi_cond_init(pi_cond_t *cond, pi_mutex_t *mutex, uint32_t flags);

int pi_cond_destroy(pi_cond_t *cond);

int pi_cond_wait(pi_cond_t *cond);

int pi_cond_timedwait(pi_cond_t *cond, const struct timespec *restrict abstime);

int pi_cond_signal(pi_cond_t *cond);

int pi_cond_broadcast(pi_cond_t *cond);

#endif // _RTPI_H
