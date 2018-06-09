/* SPDX-License-Identifier: LGPL-2.1-only */
/* Copyright © 2018 VMware, Inc. All Rights Reserved. */

#ifndef _RTPI_INTERNAL_H
#define _RTPI_INTERNAL_H

#include "rtpi.h"

typedef struct pi_mutex {
	unsigned int futex;
	unsigned int flags;
} pi_mutex_t;

typedef struct pi_cond {
	pthread_cond_t cond;
	pi_mutex_t *mutex;
} pi_cond_t;

#endif // _RTPI_INTERNAL_H
