/* Copyright (C) 2003-2019 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2003.

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
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>

#include "rtpi.h"

static char fname[] = "/tmp/tst-cond12-XXXXXX";
static int fd;

static void prepare(int argc, char* argv[])
{
	fd = mkstemp(fname);
	if (fd == -1) {
		printf("mkstemp failed: %m\n");
		exit(1);
	}

	if (ftruncate(fd, 1000) < 0) {
		printf("ftruncate failed: %m\n");
		exit(1);
	}
}

static void cleanup(void)
{
	remove(fname);
}

static int do_test(void)
{
	struct {
		pi_mutex_t m;
		pi_cond_t c;
		int var;
	} *p =
	    mmap(NULL, sizeof(*p), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (p == MAP_FAILED) {
		printf("initial mmap failed: %m\n");
		return 1;
	}

	if (pi_mutex_init(&p->m, RTPI_MUTEX_PSHARED) != 0) {
		puts("mutex_init failed");
		return 1;
	}

	if (pi_cond_init(&p->c, RTPI_COND_PSHARED) != 0) {
		puts("mutex_init failed");
		return 1;
	}

	if (pi_mutex_lock(&p->m) != 0) {
		puts("initial mutex_lock failed");
		return 1;
	}

	p->var = 42;

	pid_t pid = fork();
	if (pid == -1) {
		printf("fork failed: %m\n");
		return 1;
	}

	if (pid == 0) {
		void *oldp = p;
		p = mmap(NULL, sizeof(*p), PROT_READ | PROT_WRITE, MAP_SHARED,
			 fd, 0);

		if (p == oldp) {
			puts("child: mapped to same address");
			kill(getppid(), SIGKILL);
			exit(1);
		}

		munmap(oldp, sizeof(*p));

		if (pi_mutex_lock(&p->m) != 0) {
			puts("child: mutex_lock failed");
			kill(getppid(), SIGKILL);
			exit(1);
		}

		p->var = 0;

#ifndef USE_COND_SIGNAL
		if (pi_cond_broadcast(&p->c, &p->m) != 0) {
			puts("child: cond_broadcast failed");
			kill(getppid(), SIGKILL);
			exit(1);
		}
#else
		if (pi_cond_signal(&p->c, &p->m) != 0) {
			puts("child: cond_signal failed");
			kill(getppid(), SIGKILL);
			exit(1);
		}
#endif

		if (pi_mutex_unlock(&p->m) != 0) {
			puts("child: mutex_unlock failed");
			kill(getppid(), SIGKILL);
			exit(1);
		}

		exit(0);
	}

	do
		pi_cond_wait(&p->c, &p->m);
	while (p->var != 0);

	if (TEMP_FAILURE_RETRY(waitpid(pid, NULL, 0)) != pid) {
		printf("waitpid failed: %m\n");
		kill(pid, SIGKILL);
		return 1;
	}

	return 0;
}

#define PREPARE prepare
#define CLEANUP_HANDLER cleanup
#include "test-driver.c"
