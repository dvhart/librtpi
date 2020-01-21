/* Copyright (C) 2002-2019 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2002.

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

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>

#include "rtpi.h"

int *condition;

static int do_test(void)
{
	size_t ps = sysconf(_SC_PAGESIZE);
	char tmpfname[] = "/tmp/tst-cond4.XXXXXX";
	char data[ps];
	void *mem;
	int fd;
	pi_mutex_t *mut1;
	pi_mutex_t *mut2;
	pi_cond_t *cond;
	pid_t pid;
	int result = 0;

	fd = mkstemp(tmpfname);
	if (fd == -1) {
		printf("cannot open temporary file: %m\n");
		return 1;
	}

	/* Make sure it is always removed.  */
	unlink(tmpfname);

	/* Create one page of data.  */
	memset(data, '\0', ps);

	/* Write the data to the file.  */
	if (write(fd, data, ps) != (ssize_t) ps) {
		puts("short write");
		return 1;
	}

	mem = mmap(NULL, ps, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (mem == MAP_FAILED) {
		printf("mmap failed: %m\n");
		return 1;
	}

	mut1 = (pi_mutex_t *) (((uintptr_t) mem
				     + __alignof(pi_mutex_t))
				    & ~(__alignof(pi_mutex_t) - 1));
	mut2 = mut1 + 1;

	cond = (pi_cond_t *) (((uintptr_t) (mut2 + 1)
				    + __alignof(pi_cond_t))
				   & ~(__alignof(pi_cond_t) - 1));

	condition = (int *)(((uintptr_t) (cond + 1) + __alignof(int))
			    & ~(__alignof(int) - 1));

	if (pi_mutex_init(mut1, RTPI_MUTEX_PSHARED) != 0) {
		puts("1st mutex_init failed");
		return 1;
	}

	if (pi_mutex_init(mut2, RTPI_MUTEX_PSHARED) != 0) {
		puts("2nd mutex_init failed");
		return 1;
	}

	if (pi_cond_init(cond, RTPI_COND_PSHARED) != 0) {
		puts("cond_init failed");
		return 1;
	}

	if (pi_mutex_lock(mut1) != 0) {
		puts("parent: 1st mutex_lock failed");
		return 1;
	}

	puts("going to fork now");
	pid = fork();
	if (pid == -1) {
		puts("fork failed");
		return 1;
	} else if (pid == 0) {
		if (pi_mutex_lock(mut2) != 0) {
			puts("child: mutex_lock failed");
			return 1;
		}

		if (pi_mutex_unlock(mut1) != 0) {
			puts("child: 1st mutex_unlock failed");
			return 1;
		}

		do
			if (pi_cond_wait(cond, mut2) != 0) {
				puts("child: cond_wait failed");
				return 1;
			}
		while (*condition == 0) ;

		if (pi_mutex_unlock(mut2) != 0) {
			puts("child: 2nd mutex_unlock failed");
			return 1;
		}

		puts("child done");
	} else {
		int status;

		if (pi_mutex_lock(mut1) != 0) {
			puts("parent: 2nd mutex_lock failed");
			return 1;
		}

		if (pi_mutex_lock(mut2) != 0) {
			puts("parent: 3rd mutex_lock failed");
			return 1;
		}

		if (pi_cond_signal(cond, mut2) != 0) {
			puts("parent: cond_signal failed");
			return 1;
		}

		*condition = 1;

		if (pi_mutex_unlock(mut2) != 0) {
			puts("parent: mutex_unlock failed");
			return 1;
		}

		puts("waiting for child");

		waitpid(pid, &status, 0);
		result |= status;

		puts("parent done");
	}

	return result;
}

#include "test-driver.c"
