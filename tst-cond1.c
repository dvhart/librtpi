/* Copyright (C) 2002, 2010, 2013 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Darren Hart <dvhltc@us.ibm.com>
   Based on pthread_cond_hang.c by Dinakar Guniguntala <dino@in.ibm.com>

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#define _GNU_SOURCE
#include <error.h>
#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include "rtpi.h"

static pi_cond_t *sig1;
static pi_mutex_t *m1;

static volatile unsigned int done = 0;

static void *low_tf (void *p)
{
	int num = (int) p;
	int err;

	/* Wait for do_test to start all the threads.  */
	err = pi_mutex_lock (m1);
	if (err != 0)
		error (EXIT_FAILURE, err, "T%d: failed to lock m1\n", num);

	err = pi_cond_wait (sig1);
	if (err != 0)
		error (EXIT_FAILURE, err, "T%d: cond_wait failed on sig1\n", num);

	err = pi_mutex_unlock (m1);
	if (err != 0)
		error (EXIT_FAILURE, err, "T%d: failed to unlock race_mut\n", num);
	printf("Leave %d\n", num);
	return NULL;
}

static int do_test (void)
{
	pthread_t tthread[20];
	pthread_attr_t attr;
	int i;
	int err;

	/* Initialize mutexes and condvars.  */
	err = pthread_attr_init (&attr);
	if (err != 0)
		error (EXIT_FAILURE, err, "parent: failed to init pthread_attr");

	m1 = pi_mutex_alloc();
	sig1 = pi_cond_alloc();

	err = pi_mutex_init (m1, 0);
	if (err != 0)
		error (EXIT_FAILURE, err, "parent: failed to init mutex m1");

	err = pi_cond_init (sig1, m1, 0);
	if (err != 0)
		error (EXIT_FAILURE, err, "parent: failed to init cond sig1");

	for (i = 0; i < 20; i++) {
		err = pthread_create (&tthread[i], &attr, low_tf, (void *)(long)i);
		if (err != 0)
			error (EXIT_FAILURE, err, "parent: failed to create low_tf");
	}

	/* Wait for the threads to start and block on their respective condvars.  */
	for (i = 0; i < 2; i++) {
		sleep (1);
		printf("Sig %d\n", i);
		err = pi_cond_signal (sig1);
		if (err != 0)
			error (EXIT_FAILURE, err, "parent: failed to signal condition");
	}
	printf("BROAD\n");
	err = pi_cond_broadcast (sig1);

	for (i = 0; i < 20; i++) {
		err = pthread_join (tthread[i], NULL);
		if (err != 0)
			error (EXIT_FAILURE, err, "join of low_tf failed");
	}

	puts ("done");

	return 0;
}

int main(void)
{
	do_test();
	return 0;
}
