/* SPDX-License-Identifier: LGPL-2.1-only */
/* Copyright © 2018 VMware, Inc. All Rights Reserved. */

#ifndef RPTI_H_INTERNAL_H
#define RPTI_H_INTERNAL_H

#include <linux/futex.h>

/*
 * PI Mutex
 */
union pi_mutex {
	struct {
		__u32	futex;
		__u32	flags;
	};
	__u8 pad[64];
} __attribute__ ((aligned(64)));

#define PI_MUTEX_INIT(f) { .futex = 0, .flags = f }

/*
 * PI Cond
 */
union pi_cond {
	struct {
		union pi_mutex	priv_mut;
		__u32		cond;
		__u32		flags;
		__u32		wake_id;
		__u32		pending_wake;
		__u32		pending_wait;
	};
	__u8 pad[128];
} __attribute__ ((aligned(64)));

#define PI_COND_INIT(f) \
	{ .priv_mut = PI_MUTEX_INIT(f) \
	, .cond = 0 \
	, .flags = f \
	, .wake_id = 0 \
	, .pending_wake = 0 \
	, .pending_wait = 0 }

#endif // RPTI_H_INTERNAL_H
