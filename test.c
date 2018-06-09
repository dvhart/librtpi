// SPDX-License-Identifier: LGPL-2.1-only
// Copyright © 2018 VMware, Inc. All Rights Reserved.

#include <pthread.h>
#include <stdio.h>

#include "rtpi.h"

int main(int argc, char *argv)
{
	pi_mutex_t *mutex;
	pi_cond_t *cond;
	int ret;

	mutex = pi_mutex_alloc();
	if (!mutex) {
		printf("ERROR: failed to allocated a mutex.\n");
		goto out;
	}
	cond = pi_cond_alloc();
	if (!cond) {
		printf("ERROR: failed to allocated a cond.\n");
		goto out;
	}
	ret = pi_mutex_init(mutex, 0x0);
	if (ret) {
		printf("ERROR: pi_mutex_init returned %d\n", ret);
		goto out;
	}

	ret = pi_cond_init(cond, mutex, 0x0);
	if (ret) {
		printf("ERROR: pi_cond_init returned %d\n", ret);
		goto out;
	}

	printf("mutex @ %p\n", mutex);
	printf("cond @ %p\n", cond);

 out:
	return ret;
}

