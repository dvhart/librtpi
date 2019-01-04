/* SPDX-License-Identifier: LGPL-2.1-only */
/* Copyright © 2018 VMware, Inc. All Rights Reserved. */

#ifndef RPTI_H_INTERNAL_H
#define RPTI_H_INTERNAL_H

#include <linux/futex.h>

#include "rtpi.h"

typedef struct pi_mutex {
	__u32	futex;
	__u32	flags;
} pi_mutex_t;

typedef struct pi_cond {
	__u32		cond;
	__u32		flags;

	pi_mutex_t	priv_mut;
	__u32		wake_id;
	__u32		pending_wake;
	__u32		pending_wait;
	pi_mutex_t	*mutex;
} pi_cond_t;

#endif // RPTI_H_INTERNAL_H
